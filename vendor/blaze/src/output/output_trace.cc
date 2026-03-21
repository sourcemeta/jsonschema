#include <sourcemeta/blaze/output_trace.h>

#include <sourcemeta/core/jsonschema.h>

#include <utility> // std::move
#include <variant> // std::visit

static auto try_vocabulary(
    const std::optional<
        std::reference_wrapper<const sourcemeta::core::SchemaFrame>> &frame,
    const sourcemeta::core::WeakPointer &evaluate_path,
    const sourcemeta::core::SchemaWalker &walker,
    const sourcemeta::core::SchemaResolver &resolver,
    const std::string &keyword_location)
    -> std::pair<bool, std::optional<sourcemeta::core::Vocabularies::URI>> {
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
  const auto &result{walker(evaluate_path.back().to_property(), vocabularies)};
  return {true, result.vocabulary};
}

namespace sourcemeta::blaze {

TraceOutput::TraceOutput(
    sourcemeta::core::SchemaWalker walker,
    sourcemeta::core::SchemaResolver resolver, Callback callback,
    sourcemeta::core::WeakPointer base,
    const std::optional<
        std::reference_wrapper<const sourcemeta::core::SchemaFrame>> &frame)
    : walker_{std::move(walker)}, resolver_{std::move(resolver)},
      base_{std::move(base)}, frame_{frame}, callback_{std::move(callback)} {}

auto TraceOutput::operator()(
    const EvaluationType type, const bool result, const Instruction &step,
    const InstructionExtra &step_metadata,
    const sourcemeta::core::WeakPointer &evaluate_path,
    const sourcemeta::core::WeakPointer &instance_location,
    const sourcemeta::core::JSON &annotation) -> void {

  const auto short_step_name{
      InstructionNames[static_cast<std::underlying_type_t<InstructionIndex>>(
          step.type)]};

  // Only resolve vocabulary on Pre callbacks and cache for Post
  if (is_annotation(step.type)) {
    if (type == EvaluationType::Pre) {
      return;
    }

    auto vocabulary{try_vocabulary(this->frame_, evaluate_path, this->walker_,
                                   this->resolver_,
                                   step_metadata.keyword_location)};
    this->vocabulary_stack_.push_back(std::move(vocabulary));
  } else if (type == EvaluationType::Pre) {
    this->vocabulary_stack_.push_back(
        try_vocabulary(this->frame_, evaluate_path, this->walker_,
                       this->resolver_, step_metadata.keyword_location));
  }

  const auto &vocabulary{this->vocabulary_stack_.back()};

  // Determine the entry type
  EntryType entry_type;
  if (is_annotation(step.type)) {
    entry_type = EntryType::Annotation;
  } else if (type == EvaluationType::Pre) {
    entry_type = EntryType::Push;
  } else if (result) {
    entry_type = EntryType::Pass;
  } else {
    entry_type = EntryType::Fail;
  }

  if (this->base_.empty()) {
    const Entry entry{.type = entry_type,
                      .name = short_step_name,
                      .instance_location = instance_location,
                      .evaluate_path = evaluate_path,
                      .keyword_location = step_metadata.keyword_location,
                      .annotation = annotation,
                      .vocabulary = vocabulary};
    this->callback_(entry);
  } else {
    auto effective_evaluate_path{evaluate_path.resolve_from(this->base_)};
    const Entry entry{.type = entry_type,
                      .name = short_step_name,
                      .instance_location = instance_location,
                      .evaluate_path = effective_evaluate_path,
                      .keyword_location = step_metadata.keyword_location,
                      .annotation = annotation,
                      .vocabulary = vocabulary};
    this->callback_(entry);
  }

  if (type == EvaluationType::Post || is_annotation(step.type)) {
    this->vocabulary_stack_.pop_back();
  }
}

} // namespace sourcemeta::blaze
