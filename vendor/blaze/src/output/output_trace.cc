#include <sourcemeta/blaze/frame.h>
#include <sourcemeta/blaze/output_trace.h>

#include <sourcemeta/blaze/foundation.h>

#include <utility> // std::move, std::to_underlying
#include <variant> // std::visit

// Exported locations are keyed by keyword location, but the frame resolver is
// keyed by the schema those locations belong to
static auto schema_of(const std::string &keyword_location) -> std::string_view {
  const auto fragment{keyword_location.find('#')};
  return fragment == std::string::npos
             ? std::string_view{keyword_location}
             : std::string_view{keyword_location}.substr(0, fragment);
}

static auto try_vocabulary_from_export(
    const sourcemeta::blaze::TraceOutput::FrameResolverJSON &frames,
    const sourcemeta::core::WeakPointer &evaluate_path,
    const sourcemeta::blaze::SchemaWalker &walker,
    const sourcemeta::blaze::SchemaResolver &resolver,
    const std::string &keyword_location)
    -> std::pair<bool, std::optional<sourcemeta::blaze::Vocabularies::URI>> {
  const auto locations{frames(schema_of(keyword_location))};
  if (!locations.has_value() || !locations.value().get().is_object() ||
      !locations.value().get().defines("static")) {
    return {false, std::nullopt};
  }

  const auto &entries{locations.value().get().at("static")};
  if (!entries.is_object() || !entries.defines(keyword_location)) {
    return {false, std::nullopt};
  }

  const auto &entry{entries.at(keyword_location)};
  if (!entry.is_object() || !entry.defines("dialect") ||
      !entry.defines("baseDialect") || !entry.at("dialect").is_string() ||
      !entry.at("baseDialect").is_string()) {
    return {false, std::nullopt};
  }

  const auto base_dialect{
      sourcemeta::blaze::to_base_dialect(entry.at("baseDialect").to_string())};
  if (!base_dialect.has_value()) {
    return {false, std::nullopt};
  }

  // An export produced elsewhere may name a dialect we cannot resolve
  try {
    const auto vocabularies{sourcemeta::blaze::vocabularies(
        resolver, base_dialect.value(), entry.at("dialect").to_string())};
    const auto &result{
        walker(evaluate_path.back().to_property(), vocabularies)};
    return {true, result.vocabulary};
  } catch (const sourcemeta::blaze::SchemaResolutionError &) {
    return {false, std::nullopt};
  }
}

static auto try_vocabulary(
    const std::optional<
        std::reference_wrapper<const sourcemeta::blaze::SchemaFrame>> &frame,
    const sourcemeta::blaze::TraceOutput::FrameResolverJSON &frames,
    const sourcemeta::core::WeakPointer &evaluate_path,
    const sourcemeta::blaze::SchemaWalker &walker,
    const sourcemeta::blaze::SchemaResolver &resolver,
    const std::string &keyword_location)
    -> std::pair<bool, std::optional<sourcemeta::blaze::Vocabularies::URI>> {
  if (evaluate_path.empty() || !evaluate_path.back().is_property()) {
    return {false, std::nullopt};
  }

  if (frames) {
    return try_vocabulary_from_export(frames, evaluate_path, walker, resolver,
                                      keyword_location);
  }

  if (!frame.has_value()) {
    return {false, std::nullopt};
  }

  const auto entry{frame.value().get().traverse(keyword_location)};
  if (!entry.has_value()) {
    return {false, std::nullopt};
  }

  const auto &vocabularies{
      frame.value().get().vocabularies(entry.value().get(), resolver)};
  const auto &result{walker(evaluate_path.back().to_property(), vocabularies)};
  return {true, result.vocabulary};
}

namespace sourcemeta::blaze {

TraceOutput::TraceOutput(
    sourcemeta::blaze::SchemaWalker walker,
    sourcemeta::blaze::SchemaResolver resolver, Callback callback,
    sourcemeta::core::WeakPointer base,
    const std::optional<
        std::reference_wrapper<const sourcemeta::blaze::SchemaFrame>> &frame)
    : walker_{std::move(walker)}, resolver_{std::move(resolver)},
      base_{std::move(base)}, frame_{frame}, callback_{std::move(callback)} {}

TraceOutput::TraceOutput(sourcemeta::blaze::SchemaWalker walker,
                         sourcemeta::blaze::SchemaResolver resolver,
                         Callback callback, sourcemeta::core::WeakPointer base,
                         FrameResolverJSON frames)
    : walker_{std::move(walker)}, resolver_{std::move(resolver)},
      base_{std::move(base)}, frame_{std::nullopt}, frames_{std::move(frames)},
      callback_{std::move(callback)} {}

auto TraceOutput::operator()(
    const EvaluationType type, const bool result, const Instruction &step,
    const InstructionExtra &step_metadata,
    const sourcemeta::core::WeakPointer &evaluate_path,
    const sourcemeta::core::WeakPointer &instance_location,
    const sourcemeta::core::JSON &annotation) -> void {

  const auto short_step_name{InstructionNames[std::to_underlying(step.type)]};

  // Only resolve vocabulary on Pre callbacks and cache for Post
  if (is_annotation(step.type)) {
    if (type == EvaluationType::Pre) {
      return;
    }

    auto vocabulary{try_vocabulary(this->frame_, this->frames_, evaluate_path,
                                   this->walker_, this->resolver_,
                                   step_metadata.keyword_location)};
    this->vocabulary_stack_.push_back(std::move(vocabulary));
  } else if (type == EvaluationType::Pre) {
    this->vocabulary_stack_.push_back(try_vocabulary(
        this->frame_, this->frames_, evaluate_path, this->walker_,
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

  if (type == EvaluationType::Post || is_annotation(step.type)) {
    this->vocabulary_stack_.pop_back();
  }
}

} // namespace sourcemeta::blaze
