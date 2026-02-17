#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/linter.h>
#include <sourcemeta/blaze/output.h>

#include <sourcemeta/core/regex.h>

#include <cassert>     // assert
#include <functional>  // std::ref
#include <sstream>     // std::ostringstream
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace sourcemeta::blaze {

static constexpr std::string_view NAME_PATTERN{"^[a-z0-9_/]+$"};

static auto validate_name(const std::string_view name) -> void {
  static const auto pattern{
      sourcemeta::core::to_regex(std::string{NAME_PATTERN})};
  assert(pattern.has_value());
  if (name.empty()) {
    throw LinterInvalidNameError(name,
                                 "The schema rule name must not be empty");
  }

  if (!sourcemeta::core::matches(pattern.value(), std::string{name})) {
    std::string message{"The schema rule name must match "};
    message += NAME_PATTERN;
    throw LinterInvalidNameError(name, message);
  }
}

static auto extract_description(const sourcemeta::core::JSON &schema)
    -> std::string {
  if (!schema.defines("description")) {
    return "";
  }

  if (schema.at("description").is_string()) {
    return schema.at("description").to_string();
  }

  std::ostringstream result;
  sourcemeta::core::stringify(schema.at("description"), result);
  return std::move(result).str();
}

static auto extract_title(const sourcemeta::core::JSON &schema) -> std::string {
  if (!schema.defines("title")) {
    throw LinterMissingNameError{};
  }

  if (!schema.at("title").is_string()) {
    std::ostringstream result;
    sourcemeta::core::stringify(schema.at("title"), result);
    throw LinterInvalidNameError(std::move(result).str(),
                                 "The schema rule title is not a string");
  }

  auto title{schema.at("title").to_string()};
  validate_name(title);
  return title;
}

SchemaRule::SchemaRule(const sourcemeta::core::JSON &schema,
                       const sourcemeta::core::SchemaWalker &walker,
                       const sourcemeta::core::SchemaResolver &resolver,
                       const Compiler &compiler,
                       const std::string_view default_dialect,
                       const std::optional<Tweaks> &tweaks)
    : sourcemeta::core::SchemaTransformRule{extract_title(schema),
                                            extract_description(schema)},
      template_{compile(schema, walker, resolver, compiler, Mode::Exhaustive,
                        default_dialect, "", "", tweaks)} {};

auto SchemaRule::condition(const sourcemeta::core::JSON &schema,
                           const sourcemeta::core::JSON &,
                           const sourcemeta::core::Vocabularies &,
                           const sourcemeta::core::SchemaFrame &,
                           const sourcemeta::core::SchemaFrame::Location &,
                           const sourcemeta::core::SchemaWalker &,
                           const sourcemeta::core::SchemaResolver &) const
    -> sourcemeta::core::SchemaTransformRule::Result {
  SimpleOutput output{schema};
  const auto result{
      this->evaluator_.validate(this->template_, schema, std::ref(output))};
  if (result) {
    return false;
  }

  std::vector<sourcemeta::core::Pointer> locations;
  for (const auto &entry : output) {
    if (!entry.instance_location.empty()) {
      locations.push_back(
          sourcemeta::core::to_pointer(entry.instance_location));
    }
  }

  if (output.cbegin() != output.cend()) {
    return {std::move(locations), std::string{output.cbegin()->message}};
  } else {
    return {std::move(locations)};
  }
}

} // namespace sourcemeta::blaze
