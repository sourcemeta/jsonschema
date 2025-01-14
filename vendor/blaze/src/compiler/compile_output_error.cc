#include <sourcemeta/blaze/compiler_output.h>

#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <algorithm> // std::any_of, std::sort
#include <cassert>   // assert
#include <iterator>  // std::back_inserter
#include <utility>   // std::move

namespace sourcemeta::blaze {

ErrorOutput::ErrorOutput(const sourcemeta::jsontoolkit::JSON &instance,
                         const sourcemeta::jsontoolkit::WeakPointer &base)
    : instance_{instance}, base_{base} {}

auto ErrorOutput::begin() const -> const_iterator {
  return this->output.begin();
}

auto ErrorOutput::end() const -> const_iterator { return this->output.end(); }

auto ErrorOutput::cbegin() const -> const_iterator {
  return this->output.cbegin();
}

auto ErrorOutput::cend() const -> const_iterator { return this->output.cend(); }

auto ErrorOutput::operator()(
    const EvaluationType type, const bool result, const Instruction &step,
    const sourcemeta::jsontoolkit::WeakPointer &evaluate_path,
    const sourcemeta::jsontoolkit::WeakPointer &instance_location,
    const sourcemeta::jsontoolkit::JSON &annotation) -> void {
  if (evaluate_path.empty()) {
    return;
  }

  assert(evaluate_path.back().is_property());

  if (type == EvaluationType::Pre) {
    assert(result);
    const auto &keyword{evaluate_path.back().to_property()};
    // To ease the output
    if (keyword == "oneOf" || keyword == "not" || keyword == "if") {
      this->mask.insert(evaluate_path);
    }
  } else if (type == EvaluationType::Post &&
             this->mask.contains(evaluate_path)) {
    this->mask.erase(evaluate_path);
  }

  // Ignore successful or masked steps
  if (result || std::any_of(this->mask.cbegin(), this->mask.cend(),
                            [&evaluate_path](const auto &entry) {
                              return evaluate_path.starts_with(entry);
                            })) {
    return;
  }

  auto effective_evaluate_path{evaluate_path.resolve_from(this->base_)};
  if (effective_evaluate_path.empty()) {
    return;
  }

  this->output.push_back(
      {describe(result, step, evaluate_path, instance_location, this->instance_,
                annotation),
       instance_location, std::move(effective_evaluate_path)});
}

} // namespace sourcemeta::blaze
