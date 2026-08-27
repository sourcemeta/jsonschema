#include <sourcemeta/blaze/foundation.h>
#include <sourcemeta/blaze/output_trace.h>

#include <utility> // std::move, std::to_underlying
#include <variant> // std::visit

namespace sourcemeta::blaze {

TraceOutput::TraceOutput(const Template &schema_template, Callback callback,
                         sourcemeta::core::WeakPointer base)
    : schema_template_{schema_template}, base_{std::move(base)},
      callback_{std::move(callback)} {}

auto TraceOutput::operator()(
    const EvaluationType type, const bool result, const Instruction &step,
    const InstructionExtra &step_metadata,
    const sourcemeta::core::WeakPointer &evaluate_path,
    const sourcemeta::core::WeakPointer &instance_location,
    const sourcemeta::core::JSON &annotation) -> void {

  const auto short_step_name{InstructionNames[std::to_underlying(step.type)]};

  if (is_annotation(step.type) && type == EvaluationType::Pre) {
    return;
  }

  // The compiler stamps the vocabulary that owns each keyword, indexed into
  // the template, where zero means the keyword has none
  const std::optional<std::string_view> vocabulary{
      step_metadata.vocabulary == 0
          ? std::nullopt
          : std::optional<std::string_view>{
                this->schema_template_.vocabularies.at(
                    step_metadata.vocabulary - 1)}};

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
                      .step = step,
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
                      .step = step,
                      .instance_location = instance_location,
                      .evaluate_path = effective_evaluate_path,
                      .keyword_location = step_metadata.keyword_location,
                      .annotation = annotation,
                      .vocabulary = vocabulary};
    this->callback_(entry);
  }
}

} // namespace sourcemeta::blaze
