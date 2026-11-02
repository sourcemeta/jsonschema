#include <sourcemeta/blaze/output_simple.h>

#include <sourcemeta/core/jsonschema.h>

#include <algorithm> // std::min, std::remove_if
#include <cassert>   // assert
#include <iterator>  // std::make_move_iterator
#include <ranges>    // std::views::reverse
#include <utility>   // std::move

namespace sourcemeta::blaze {

SimpleOutput::SimpleOutput(const Template &schema_template,
                           const sourcemeta::core::JSON &instance,
                           sourcemeta::core::WeakPointer base)
    : schema_template_{schema_template}, instance_{instance},
      base_{std::move(base)} {}

auto SimpleOutput::mask_kind(const Instruction &step) noexcept -> MaskKind {
  switch (step.type) {
    case InstructionIndex::LogicalOr:
    case InstructionIndex::LogicalXor:
      return MaskKind::Disjunction;
    case InstructionIndex::LoopContains:
      return MaskKind::Element;
    case InstructionIndex::LogicalCondition:
    case InstructionIndex::LogicalNot:
    case InstructionIndex::LogicalNotEvaluate:
      return MaskKind::Subschema;
    default:
      return MaskKind::None;
  }
}

auto SimpleOutput::destination(
    const sourcemeta::core::WeakPointer &evaluate_path) -> container_type * {
  container_type *result{&this->output_};
  for (auto &mask_entry : this->mask_) {
    if (!evaluate_path.starts_with(mask_entry.evaluate_path)) {
      continue;
    }

    // Whatever a subschema mask absorbs never surfaces, however the
    // instructions below it turn out, so there is nothing to collect and
    // no message worth assembling for any of it
    if (mask_entry.kind == MaskKind::Subschema) {
      return nullptr;
    }

    // The innermost branching keyword that encloses the instruction is the
    // one that gets to decide whether its failure surfaces, so keep looking
    // for a closer one rather than settling for the first match
    result = &mask_entry.buffered_traces;
  }

  return result;
}

auto SimpleOutput::begin() const -> const_iterator {
  return this->output_.begin();
}

auto SimpleOutput::end() const -> const_iterator { return this->output_.end(); }

auto SimpleOutput::cbegin() const -> const_iterator {
  return this->output_.cbegin();
}

auto SimpleOutput::cend() const -> const_iterator {
  return this->output_.cend();
}

auto SimpleOutput::operator()(
    const EvaluationType type, const bool result, const Instruction &step,
    const InstructionExtra &step_metadata,
    const sourcemeta::core::WeakPointer &evaluate_path,
    const sourcemeta::core::WeakPointer &instance_location,
    const sourcemeta::core::JSON &annotation) -> void {
  if (evaluate_path.empty()) {
    return;
  }

  // Fast path: passing non-annotation instructions that are not
  // closing a mask entry can be skipped entirely
  if (result && !is_annotation(step.type)) {
    if (type == EvaluationType::Pre) {
      const auto kind{mask_kind(step)};
      if (kind != MaskKind::None) {
        this->mask_.push_back({.evaluate_path = evaluate_path,
                               .instance_location = instance_location,
                               .kind = kind,
                               .annotations_mark = this->annotations_.size(),
                               .buffered_traces = {}});
      }
    } else if (type == EvaluationType::Post && !this->mask_.empty() &&
               this->mask_.back().evaluate_path == evaluate_path &&
               this->mask_.back().instance_location == instance_location) {
      this->mask_.pop_back();
    }

    return;
  }

  auto effective_evaluate_path{evaluate_path.resolve_from(this->base_)};
  if (effective_evaluate_path.empty()) {
    return;
  }

  if (is_annotation(step.type)) {
    if (type == EvaluationType::Post) {
      this->annotations_.push_back(
          {.instance_location = instance_location,
           .evaluate_path = std::move(effective_evaluate_path),
           .schema_location = step_metadata.keyword_location,
           .value = annotation,
           .is_wrapped = is_wrapped_annotation(step.type)});
    }

    return;
  }

  if (type == EvaluationType::Pre) {
    assert(result);
    // To ease the output
    const auto kind{mask_kind(step)};
    if (kind != MaskKind::None) {
      this->mask_.push_back({.evaluate_path = evaluate_path,
                             .instance_location = instance_location,
                             .kind = kind,
                             .annotations_mark = this->annotations_.size(),
                             .buffered_traces = {}});
    }
  } else if (type == EvaluationType::Post) {
    const auto mask_it{std::ranges::find_if(
        this->mask_, [&](const MaskEntry &mask_entry) -> bool {
          return mask_entry.evaluate_path == evaluate_path &&
                 mask_entry.instance_location == instance_location;
        })};
    if (mask_it != this->mask_.end()) {
      // Present unexpected traces only when needed, which is when the
      // branching keyword ran out of ways to hold and a subschema mask is
      // not the one that absorbed them
      auto buffered{std::move(mask_it->buffered_traces)};
      const auto surfaces{!result && mask_it->kind != MaskKind::Subschema};
      this->mask_.erase(mask_it);

      // The traces move up to whichever branching keyword now encloses the
      // one that just closed, so that an outer keyword absorbing this
      // failure still gets to drop them
      if (surfaces && !buffered.empty()) {
        auto *target{this->destination(evaluate_path)};
        if (target != nullptr) {
#ifdef __cpp_lib_containers_ranges
          target->append_range(std::move(buffered));
#else
          target->insert(target->end(),
                         std::make_move_iterator(buffered.begin()),
                         std::make_move_iterator(buffered.end()));
#endif
        }
      }
    }
  }

  if (result) {
    return;
  }

  if (type == EvaluationType::Post && !this->annotations_.empty()) {
    // A failure inside a branching keyword discards every annotation that
    // the failing unit collected, including the ones at instance locations
    // below the unit's own instance location. The unit is the disjunction
    // branch for `anyOf` and `oneOf`, the application of the subschema to
    // the array element for `contains`, and the entire subschema for `if`
    // and negations. A failure outside of every branching keyword cannot
    // be absorbed, so the overall result is bound to be false, in which
    // case no annotations may be reported at all
    const MaskEntry *unit{nullptr};
    for (const auto &mask_entry : std::views::reverse(this->mask_)) {
      if (evaluate_path.starts_with(mask_entry.evaluate_path) &&
          instance_location.starts_with(mask_entry.instance_location)) {
        unit = &mask_entry;
        break;
      }
    }

    if (unit != nullptr) {
      auto evaluate_prefix_size{unit->evaluate_path.size()};
      auto instance_prefix_size{unit->instance_location.size()};
      switch (unit->kind) {
        case MaskKind::Disjunction:
          evaluate_prefix_size =
              std::min(evaluate_prefix_size + 1, evaluate_path.size());
          break;
        case MaskKind::Element:
          instance_prefix_size =
              std::min(instance_prefix_size + 1, instance_location.size());
          break;
        default:
          break;
      }

      // Every annotation the failing unit collected, including the ones
      // that the equality check below would match, sits at or beyond the
      // mark the unit's branching keyword recorded, so older entries do
      // not need to be scanned at all
      assert(unit->annotations_mark <= this->annotations_.size());
      const auto from{this->annotations_.begin() +
                      static_cast<std::ptrdiff_t>(unit->annotations_mark)};
      this->annotations_.erase(
          std::remove_if(
              from, this->annotations_.end(),
              [&](const AnnotationEntry &entry) -> bool {
                return (entry.evaluate_path.starts_with_initial(
                            evaluate_path) &&
                        entry.instance_location == instance_location) ||
                       (entry.evaluate_path.shares_prefix(
                            evaluate_path, evaluate_prefix_size) &&
                        entry.instance_location.shares_prefix(
                            instance_location, instance_prefix_size));
              }),
          this->annotations_.end());
    } else {
      this->annotations_.clear();
    }
  }

  if (step.type == InstructionIndex::LogicalCondition) {
    return;
  }
  auto *target{this->destination(evaluate_path)};
  if (target == nullptr) {
    return;
  }

  target->push_back(
      {.message = describe(this->schema_template_, result, step, evaluate_path,
                           instance_location, this->instance_, annotation),
       .instance_location = instance_location,
       .evaluate_path = std::move(effective_evaluate_path),
       .schema_location = step_metadata.keyword_location});
}

} // namespace sourcemeta::blaze
