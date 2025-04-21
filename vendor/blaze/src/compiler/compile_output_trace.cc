#include <sourcemeta/blaze/compiler_output.h>

#include <sourcemeta/core/jsonschema.h>

#include <utility> // std::move
#include <variant> // std::visit

static auto step_name(const sourcemeta::blaze::Instruction &instruction)
    -> std::string_view {
  return sourcemeta::blaze::InstructionNames
      [static_cast<std::underlying_type_t<sourcemeta::blaze::InstructionIndex>>(
          instruction.type)];
}

static auto try_vocabulary(
    const std::optional<
        std::reference_wrapper<const sourcemeta::core::SchemaFrame>> &frame,
    const sourcemeta::core::WeakPointer &evaluate_path,
    const sourcemeta::core::SchemaWalker &walker,
    const sourcemeta::core::SchemaResolver &resolver,
    const std::string &keyword_location)
    -> std::pair<bool, std::optional<std::string>> {
  if (!frame.has_value() || evaluate_path.empty() ||
      !evaluate_path.back().is_property()) {
    return {false, std::nullopt};
  }

  const auto entry{frame.value().get().traverse(keyword_location)};
  if (!entry.has_value()) {
    return {false, std::nullopt};
  }

  const auto vocabularies{
      frame.value().get().vocabularies(entry.value().get(), resolver)};
  const auto result{walker(evaluate_path.back().to_property(), vocabularies)};
  return {true, result.vocabulary};
}

namespace sourcemeta::blaze {

TraceOutput::TraceOutput(
    const sourcemeta::core::SchemaWalker &walker,
    const sourcemeta::core::SchemaResolver &resolver,
    const sourcemeta::core::WeakPointer &base,
    const std::optional<
        std::reference_wrapper<const sourcemeta::core::SchemaFrame>> &frame)
    : walker_{walker}, resolver_{resolver}, base_{base}, frame_{frame} {}

auto TraceOutput::begin() const -> const_iterator {
  return this->output.begin();
}

auto TraceOutput::end() const -> const_iterator { return this->output.end(); }

auto TraceOutput::cbegin() const -> const_iterator {
  return this->output.cbegin();
}

auto TraceOutput::cend() const -> const_iterator { return this->output.cend(); }

auto TraceOutput::operator()(
    const EvaluationType type, const bool result, const Instruction &step,
    const sourcemeta::core::WeakPointer &evaluate_path,
    const sourcemeta::core::WeakPointer &instance_location,
    const sourcemeta::core::JSON &annotation) -> void {

  const auto short_step_name{step_name(step)};
  auto effective_evaluate_path{evaluate_path.resolve_from(this->base_)};

  // Attempt to get vocabulary information if we can get it
  auto vocabulary{try_vocabulary(this->frame_, evaluate_path, this->walker_,
                                 this->resolver_, step.keyword_location)};

  if (is_annotation(step.type)) {
    if (type == EvaluationType::Pre) {
      return;
    } else {
      this->output.push_back(
          {EntryType::Annotation, short_step_name, instance_location,
           std::move(effective_evaluate_path), step.keyword_location,
           annotation, std::move(vocabulary)});
    }
  } else if (type == EvaluationType::Pre) {
    this->output.push_back({EntryType::Push, short_step_name, instance_location,
                            std::move(effective_evaluate_path),
                            step.keyword_location, std::nullopt,
                            std::move(vocabulary)});
  } else if (result) {
    this->output.push_back({EntryType::Pass, short_step_name, instance_location,
                            std::move(effective_evaluate_path),
                            step.keyword_location, std::nullopt,
                            std::move(vocabulary)});
  } else {
    this->output.push_back({EntryType::Fail, short_step_name, instance_location,
                            std::move(effective_evaluate_path),
                            step.keyword_location, std::nullopt,
                            std::move(vocabulary)});
  }
}

} // namespace sourcemeta::blaze
