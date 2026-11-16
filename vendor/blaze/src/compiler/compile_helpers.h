#ifndef SOURCEMETA_BLAZE_COMPILER_COMPILE_HELPERS_H_
#define SOURCEMETA_BLAZE_COMPILER_COMPILE_HELPERS_H_

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/uri.h>

#include <algorithm> // std::ranges::find, std::ranges::all_of, std::find_if, std::rotate
#include <cassert>     // assert
#include <functional>  // std::cref
#include <iterator>    // std::distance, std::next
#include <limits>      // std::numeric_limits
#include <optional>    // std::optional
#include <regex>       // std::regex, std::regex_match, std::smatch
#include <string_view> // std::string_view
#include <utility>     // std::move

#include "upstream.h"

namespace sourcemeta::blaze {

// Static keyword strings for use in DynamicContext references
// NOLINTBEGIN(cert-err58-cpp,bugprone-throwing-static-initialization)
static const sourcemeta::core::JSON::String KEYWORD_PROPERTIES{"properties"};
static const sourcemeta::core::JSON::String KEYWORD_THEN{"then"};
static const sourcemeta::core::JSON::String KEYWORD_ELSE{"else"};
// NOLINTEND(cert-err58-cpp,bugprone-throwing-static-initialization)

// Helper to create a single-element WeakPointer from a property name reference
inline auto make_weak_pointer(const std::string &property)
    -> sourcemeta::core::WeakPointer {
  sourcemeta::core::WeakPointer result;
  result.push_back(std::cref(property));
  return result;
}

// Helper to create a two-element WeakPointer from property name and index
inline auto make_weak_pointer(const std::string &property,
                              const std::size_t index)
    -> sourcemeta::core::WeakPointer {
  sourcemeta::core::WeakPointer result;
  result.push_back(std::cref(property));
  result.push_back(index);
  return result;
}

// Helper to create a two-element WeakPointer from two property names
inline auto make_weak_pointer(const std::string &property1,
                              const std::string &property2)
    -> sourcemeta::core::WeakPointer {
  sourcemeta::core::WeakPointer result;
  result.push_back(std::cref(property1));
  result.push_back(std::cref(property2));
  return result;
}

inline auto schema_resource_id(const std::vector<std::string> &resources,
                               const std::string_view resource) -> std::size_t {
  const auto iterator{std::ranges::find(
      resources, sourcemeta::core::URI::canonicalize(resource))};
  if (iterator == resources.cend()) {
    assert(resource.empty());
    return 0;
  }

  return 1 +
         static_cast<std::size_t>(std::distance(resources.cbegin(), iterator));
}

// Intern the vocabulary that owns a keyword, as an index into
// Template::vocabularies where zero means the keyword has none
inline auto vocabulary_intern(
    std::vector<sourcemeta::core::SchemaVocabularies::URI> &vocabularies,
    const sourcemeta::core::SchemaWalker &walker,
    const std::string_view keyword,
    const sourcemeta::core::SchemaVocabularies &active) -> std::size_t {
  const auto &result{walker(keyword, active)};
  if (!result.vocabulary.has_value()) {
    return 0;
  }

  // Intern on the vocabulary itself rather than on its string form, as the
  // known ones are a single byte and comparing them allocates nothing
  const auto vocabulary{
      sourcemeta::core::schema_vocabulary_uri(result.vocabulary.value())};
  const auto iterator{std::ranges::find(vocabularies, vocabulary)};
  if (iterator == vocabularies.end()) {
    vocabularies.push_back(vocabulary);
    return vocabularies.size();
  }

  return 1 + static_cast<std::size_t>(
                 std::distance(vocabularies.begin(), iterator));
}

inline auto vocabulary_id(
    std::vector<sourcemeta::core::SchemaVocabularies::URI> &vocabularies,
    const sourcemeta::core::SchemaFrame &frame,
    const sourcemeta::core::SchemaWalker &walker,
    const std::string_view keyword,
    const sourcemeta::core::WeakPointer &relative_pointer,
    const sourcemeta::core::URI &base,
    const sourcemeta::core::SchemaVocabularies &active) -> std::size_t {
  if (!keyword.empty()) {
    return vocabulary_intern(vocabularies, walker, keyword, active);
  }

  // Instructions that a keyword emits on its own behalf, such as annotation
  // emitters, carry no keyword of their own. They sit at the location of the
  // keyword that produced them, so its last token names that keyword
  if (relative_pointer.empty() || !relative_pointer.back().is_property()) {
    return 0;
  }

  // Except that a keyword compiler may build a context pointing at a
  // subschema rather than at a keyword, in which case the last token is a
  // property name that may well collide with a keyword. The token names a
  // keyword exactly when what contains it is itself a schema, so ask the
  // frame about the parent rather than guessing. Note we cannot ask about
  // the token itself, as an applicator that takes a schema, such as
  // `additionalProperties`, sits at a subschema location of its own
  const auto parent{
      frame.traverse(to_uri(relative_pointer.initial(), base).recompose())};
  if (!parent.has_value() ||
      (parent.value().get().type !=
           sourcemeta::core::SchemaFrame::LocationType::Subschema &&
       parent.value().get().type !=
           sourcemeta::core::SchemaFrame::LocationType::Resource)) {
    return 0;
  }

  return vocabulary_intern(vocabularies, walker,
                           relative_pointer.back().to_property(), active);
}

// Instantiate a value-oriented step with a custom resource
inline auto make_with_resource(const InstructionIndex type,
                               const KeywordContext &context,
                               const Value &value, const std::string &resource)
    -> Instruction {
  const auto schema_location{
      context.keyword().empty()
          ? to_pointer(context.target.base_schema_location)
          : to_pointer(context.target.base_schema_location)
                .concat(context.keyword())};
  const auto extra_index{context.global.extra.size()};
  context.global.extra.push_back(
      {.relative_schema_location = schema_location,
       .keyword_location =
           to_uri(context.local.relative_pointer, context.base()).recompose(),
       .schema_resource =
           schema_resource_id(context.global.resources, resource),
       .vocabulary =
           vocabulary_id(context.global.vocabularies, context.global.frame,
                         context.global.walker, context.keyword(),
                         context.local.relative_pointer, context.base(),
                         context.vocabularies()),
       .message = 0,
       .message_detail = 0});
  return {.type = type,
          .relative_instance_location =
              to_pointer(context.target.base_instance_location),
          .value = value,
          .children = {},
          .extra_index = extra_index};
}

// Instantiate a value-oriented step
inline auto make(const InstructionIndex type, const KeywordContext &context,
                 const Value &value) -> Instruction {
  return make_with_resource(type, context, value, context.base().recompose());
}

// Instantiate an applicator step
inline auto make(const InstructionIndex type, const KeywordContext &context,
                 Value &&value, Instructions &&children) -> Instruction {
  const auto schema_location{
      context.keyword().empty()
          ? to_pointer(context.target.base_schema_location)
          : to_pointer(context.target.base_schema_location)
                .concat(context.keyword())};
  const auto extra_index{context.global.extra.size()};
  context.global.extra.push_back(
      {.relative_schema_location = schema_location,
       .keyword_location =
           to_uri(context.local.relative_pointer, context.base()).recompose(),
       .schema_resource = schema_resource_id(context.global.resources,
                                             context.base().recompose()),
       .vocabulary =
           vocabulary_id(context.global.vocabularies, context.global.frame,
                         context.global.walker, context.keyword(),
                         context.local.relative_pointer, context.base(),
                         context.vocabularies()),
       .message = 0,
       .message_detail = 0});
  return {.type = type,
          .relative_instance_location =
              to_pointer(context.target.base_instance_location),
          .value = std::move(value),
          .children = std::move(children),
          .extra_index = extra_index};
}

inline auto unroll(const KeywordContext &context, const Instruction &step,
                   const sourcemeta::core::WeakPointer &base_instance_location =
                       sourcemeta::core::EMPTY_WEAK_POINTER) -> Instruction {
  auto source_extra{context.global.extra[step.extra_index]};
  const auto extra_index{context.global.extra.size()};
  context.global.extra.push_back(std::move(source_extra));
  return {.type = step.type,
          .relative_instance_location =
              to_pointer(base_instance_location)
                  .concat(step.relative_instance_location),
          .value = step.value,
          .children = {},
          .extra_index = extra_index};
}

inline auto rephrase(const KeywordContext &context, const InstructionIndex type,
                     const Instruction &step) -> Instruction {
  auto source_extra{context.global.extra[step.extra_index]};
  const auto extra_index{context.global.extra.size()};
  context.global.extra.push_back(std::move(source_extra));
  return {.type = type,
          .relative_instance_location = step.relative_instance_location,
          .value = step.value,
          .children = {},
          .extra_index = extra_index};
}

// Whether the bound fits the integer the instructions carry. One that does not
// sits beyond the size of anything that can be held in memory, so the keyword
// compilers say such a bound outright rather than convert a value that does
// not fit. The comparison reads the number by its value, so it holds whichever
// way the literal came to be stored
inline auto bound_is_representable(const sourcemeta::core::JSON &value)
    -> bool {
  static const sourcemeta::core::JSON LIMIT{
      std::numeric_limits<sourcemeta::core::JSON::Integer>::max()};
  return value <= LIMIT;
}

// Each of these dialects pairs a validation vocabulary with a hyper-schema one
// that shares every keyword under discussion here, so asking which rung a
// schema is written against means asking about both. Note these answer about
// the vocabularies in force rather than about the base dialect, which is why
// the OpenAPI 3.0 Schema Object answers no to all of them: it extends the
// Draft 4 base dialect structurally while defining a keyword set of its own
inline auto is_draft3(const sourcemeta::core::SchemaVocabularies &vocabularies)
    -> bool {
  using Known = sourcemeta::core::SchemaVocabularies::Known;
  return vocabularies.contains_any(
      {Known::JSON_SCHEMA_DRAFT_3, Known::JSON_SCHEMA_DRAFT_3_HYPER});
}

inline auto is_draft4(const sourcemeta::core::SchemaVocabularies &vocabularies)
    -> bool {
  using Known = sourcemeta::core::SchemaVocabularies::Known;
  return vocabularies.contains_any(
      {Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER});
}

inline auto is_draft6(const sourcemeta::core::SchemaVocabularies &vocabularies)
    -> bool {
  using Known = sourcemeta::core::SchemaVocabularies::Known;
  return vocabularies.contains_any(
      {Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER});
}

inline auto is_draft7(const sourcemeta::core::SchemaVocabularies &vocabularies)
    -> bool {
  using Known = sourcemeta::core::SchemaVocabularies::Known;
  return vocabularies.contains_any(
      {Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER});
}

inline auto
is_openapi_3_0(const sourcemeta::core::SchemaVocabularies &vocabularies)
    -> bool {
  return vocabularies.contains(
      sourcemeta::core::SchemaVocabularies::Known::OPENAPI_3_0_BASE);
}

// Draft 6 introduced boolean schemas. Draft 4 and earlier have none, and the
// only places they accept a boolean are `additionalProperties` and
// `additionalItems`, whose own definitions spell that out. The OpenAPI 3.0
// Schema Object has none either, and offers a boolean at
// `additionalProperties` alone
inline auto
booleans_are_schemas(const sourcemeta::core::SchemaVocabularies &vocabularies)
    -> bool {
  return !is_draft3(vocabularies) && !is_draft4(vocabularies) &&
         !is_openapi_3_0(vocabularies);
}

// The same as `is_schema_array`, for the callers that have already settled how
// many schemas the keyword may hold
inline auto
all_are_schemas(const sourcemeta::core::JSON &value,
                const sourcemeta::core::SchemaVocabularies &vocabularies)
    -> bool {
  return std::ranges::all_of(
      value.as_array(), [&vocabularies](const auto &entry) -> bool {
        return sourcemeta::core::schema_is_schema(entry, vocabularies);
      });
}

// The meta-schema describes these as a non-empty array of schemas, other than
// the OpenAPI 3.0 Schema Object, which leaves an empty one well-formed. A
// keyword that does not satisfy that is not a constraint at all, so we ignore
// it rather than compile part of it or turn it into a failure
inline auto
is_schema_array(const sourcemeta::core::JSON &value,
                const sourcemeta::core::SchemaVocabularies &vocabularies,
                const bool allow_empty = false) -> bool {
  return value.is_array() && (allow_empty || !value.empty()) &&
         all_are_schemas(value, vocabularies);
}

// Whether a boolean stands for a schema where this keyword holds its value, as
// the dialects that have no boolean schemas still define `additionalProperties`
// and `additionalItems` to accept one in place of a schema
inline auto
is_keyword_schema(const sourcemeta::core::JSON &value,
                  const sourcemeta::core::SchemaVocabularies &vocabularies,
                  const std::string_view keyword) -> bool {
  if (value.is_boolean() &&
      (keyword == "additionalProperties" || keyword == "additionalItems")) {
    return true;
  }

  return sourcemeta::core::schema_is_schema(value, vocabularies);
}

// JSON Schema spells `required` and its relatives as an array of property
// names. Which of them asks for those names to be unique is a question per
// keyword and per dialect, so it is not part of this shape: `required` from
// Draft 4, `dependentRequired` from 2019-09, and the array form of
// `dependencies` from Draft 4 to Draft 7 all ask for unique entries, while the
// array form of Draft 3 `dependencies` asks for none
inline auto is_property_name_array(const sourcemeta::core::JSON &value)
    -> bool {
  return value.is_array() &&
         std::ranges::all_of(value.as_array(), [](const auto &entry) -> bool {
           return entry.is_string();
         });
}

// Draft 3 to 7 let `$ref` replace every sibling keyword, so nothing those
// siblings say ever takes effect, and the OpenAPI 3.0 Schema Object answers
// with Draft 4 here. Core's schema walker frames nothing under one of those
// drafts either, which is what keeps such a sibling from compiling at all
inline auto ref_overrides_sibling_keywords(const KeywordContext &context)
    -> bool {
  return context.schema().is_object() && context.schema().defines("$ref") &&
         (is_draft3(context.vocabularies()) ||
          is_draft4(context.vocabularies()) ||
          is_draft6(context.vocabularies()) ||
          is_draft7(context.vocabularies()) ||
          is_openapi_3_0(context.vocabularies()));
}

// The OpenAPI 3.0 Schema Object spells a reference as a Reference Object, whose
// every other member the specification says to ignore. The shape of such a
// member is therefore not this dialect's to judge at all, and its own
// meta-schema agrees: it accepts whatever stands beside a `$ref`. Every other
// dialect here lets `$ref` override its siblings without excusing them from
// being well-formed, which their meta-schemas do go on to check
inline auto ref_object_ignores_siblings(
    const sourcemeta::core::SchemaVocabularies &vocabularies) -> bool {
  return is_openapi_3_0(vocabularies);
}

// Note that the size keywords that let the type level fuse their bound accept
// any integral number, including reals with no fractional part like `2.0`, so
// this must recognise those too or the fused bound would be silently dropped
inline auto
unsigned_integer_property(const sourcemeta::core::JSON &document,
                          const sourcemeta::core::JSON::String &property)
    -> std::optional<std::size_t> {
  if (document.defines(property) && document.at(property).is_integral()) {
    // A real or decimal may spell an integer too large to represent, and such a
    // bound sits so far beyond any instance that we ignore it, just as a
    // non-integral bound is ignored, rather than let the conversion raise
    try {
      const auto value{document.at(property).as_integer()};
      // A negative bound is invalid, and the keyword compilers ignore it, so
      // the fused form must ignore it too
      if (value < 0) {
        return std::nullopt;
      }

      return static_cast<std::size_t>(value);
    } catch (const sourcemeta::core::JSONIntegerRangeError &) {
      return std::nullopt;
    }
  }

  return std::nullopt;
}

inline auto
unsigned_integer_property(const sourcemeta::core::JSON &document,
                          const sourcemeta::core::JSON::String &property,
                          const std::size_t otherwise) -> std::size_t {
  return unsigned_integer_property(document, property).value_or(otherwise);
}

// A schema context only knows where it sits within the schema resource that
// encloses it, while an error must report where the problem is within the
// document that the schema came from. Prepending the pointer of that resource
// bridges the two, so that a consumer can resolve the location against the
// document it parsed without keeping a frame of its own
inline auto
absolute_schema_location(const Context &context,
                         const sourcemeta::core::URI &base,
                         const sourcemeta::core::WeakPointer &relative_pointer)
    -> sourcemeta::core::Pointer {
  return to_pointer(sourcemeta::core::schema_frame_absolute_pointer(
      context.frame, base.recompose(), relative_pointer));
}

inline auto absolute_schema_location(const KeywordContext &context)
    -> sourcemeta::core::Pointer {
  return absolute_schema_location(context.global, context.base(),
                                  context.local.relative_pointer);
}

// The messages that compilation refuses a keyword with. The location such an
// error carries names the keyword, so these read as statements about it
static constexpr auto EXPECTED_STRING{
    "This keyword was expected to be set to a string"};
static constexpr auto EXPECTED_INTEGER{
    "This keyword was expected to be set to an integer"};
static constexpr auto EXPECTED_NUMBER{
    "This keyword was expected to be set to a number"};
static constexpr auto EXPECTED_BOOLEAN{
    "This keyword was expected to be set to a boolean"};
static constexpr auto EXPECTED_ARRAY{
    "This keyword was expected to be set to an array"};
static constexpr auto EXPECTED_OBJECT{
    "This keyword was expected to be set to an object"};
static constexpr auto EXPECTED_DIALECT_OBJECT{
    "This keyword was expected to be set to an object of the shape this "
    "dialect defines"};
static constexpr auto EXPECTED_NON_NEGATIVE_INTEGER{
    "This keyword was expected to be set to a non-negative integer"};
static constexpr auto EXPECTED_SCHEMA{
    "This keyword was expected to be set to a valid schema"};
static constexpr auto EXPECTED_SCHEMA_OR_ARRAY{
    "This keyword was expected to be set to a valid schema or to an array "
    "of them"};
static constexpr auto EXPECTED_SCHEMA_ARRAY{
    "This keyword was expected to be set to an array of valid schemas"};
static constexpr auto EXPECTED_SCHEMA_ARRAY_NON_EMPTY{
    "This keyword was expected to be set to a non-empty array of valid "
    "schemas"};
static constexpr auto EXPECTED_SCHEMA_OBJECT{
    "This keyword was expected to be set to an object whose values are "
    "valid schemas"};
static constexpr auto EXPECTED_PROPERTY_NAME_ARRAY{
    "This keyword was expected to be set to an array of unique property "
    "names"};
static constexpr auto EXPECTED_PROPERTY_NAME_ARRAYS{
    "Every value of this keyword was expected to be an array of unique "
    "property names"};
static constexpr auto EXPECTED_POSITIVE{
    "This keyword was expected to be set to a number greater than zero"};
static constexpr auto EXPECTED_NON_NEGATIVE{
    "This keyword was expected to be set to a non-negative number"};
static constexpr auto EXPECTED_TYPE_NAMES{
    "This keyword was expected to be set to a known type name, or to a "
    "non-empty array of unique ones"};
static constexpr auto EXPECTED_UNIQUE_VALUES{
    "This keyword was expected to be set to a non-empty array of unique "
    "values"};
static constexpr auto EXPECTED_NON_EMPTY_VALUES{
    "This keyword was expected to be set to a non-empty array of values"};
static constexpr auto EXPECTED_DEPENDENCIES{
    "Every value of this keyword was expected to be a valid schema or an "
    "array of unique property names"};
static constexpr auto EXPECTED_DEPENDENCIES_DRAFT3{
    "Every value of this keyword was expected to be a valid schema, a "
    "property name, or an array of property names"};
static constexpr auto UNSUPPORTED_VALUE_FOR_DIALECT{
    "The value this keyword is set to is not supported by this dialect"};
static constexpr auto PROHIBITED_KEYWORD_FOR_DIALECT{
    "This keyword is prohibited by this dialect"};

// Refuse the keyword that the schema context points at. The location the error
// carries names that keyword, so every message passed here reads as a
// statement about it
[[noreturn]] inline auto throw_keyword_error(const KeywordContext &context,
                                             const char *const message)
    -> void {
  throw CompilerError{context.base(), absolute_schema_location(context),
                      message};
}

// Same bridging as above, but keeping the pointer weak, as every caller only
// wants to address the frame with it rather than to hand it out
inline auto
absolute_schema_pointer(const Context &context,
                        const sourcemeta::core::URI &base,
                        const sourcemeta::core::WeakPointer &relative_pointer)
    -> sourcemeta::core::WeakPointer {
  return sourcemeta::core::schema_frame_absolute_pointer(
      context.frame, base.recompose(), relative_pointer);
}

inline auto absolute_schema_pointer(const KeywordContext &context)
    -> sourcemeta::core::WeakPointer {
  return absolute_schema_pointer(context.global, context.base(),
                                 context.local.relative_pointer);
}

// Whether the current keyword value, as a schema, contains any nested
// subschema. Note that while the schema of the schema context of a keyword
// compiler is the parent subschema, its relative pointer already targets
// the keyword value, so the frame entry we look up corresponds to the
// keyword value and not to the parent subschema
inline auto defines_nested_subschemas(const KeywordContext &context) -> bool {
  return sourcemeta::core::schema_frame_has_subschemas_under(
      context.global.frame, absolute_schema_pointer(context));
}

// TODO: Get rid of this given the new Core regex optimisations
inline auto pattern_as_prefix(const std::string &pattern)
    -> std::optional<std::string> {
  static const std::regex STARTS_WITH_REGEX{R"(^\^([a-zA-Z0-9-_/]+)$)"};
  std::smatch matches;
  if (std::regex_match(pattern, matches, STARTS_WITH_REGEX)) {
    return matches[1].str();
  }
  return std::nullopt;
}

inline auto recursive_template_size(const Instructions &steps) -> std::size_t {
  std::size_t result{steps.size()};
  for (const auto &variant : steps) {
    result += recursive_template_size(variant.children);
  }

  return result;
}

inline auto make_property(const ValueString &property) -> ValueProperty {
  static const sourcemeta::core::PropertyHashJSON<ValueString> HASHER;
  return {property, HASHER(property)};
}

inline auto requires_evaluation(const Context &context,
                                const sourcemeta::core::WeakPointer &pointer)
    -> bool {
  for (const auto &unevaluated : context.unevaluated) {
    if (unevaluated.second.unresolved ||
        unevaluated.second.dynamic_dependencies.contains(pointer)) {
      return true;
    }

    for (const auto &dependency : unevaluated.second.dynamic_dependencies) {
      if (dependency.starts_with(pointer)) {
        return true;
      }
    }
  }

  return false;
}

inline auto requires_evaluation(const KeywordContext &context) -> bool {
  return requires_evaluation(context.global, absolute_schema_pointer(context));
}

inline auto annotations_enabled(const KeywordContext &context,
                                const std::string_view keyword) -> bool {
  if (context.global.tweaks.annotations.has_value()) {
    return context.global.tweaks.annotations.value().contains(keyword);
  }

  return context.mode() == Mode::Exhaustive;
}

// The same, for the keyword that the context already names
inline auto annotations_enabled(const KeywordContext &context) -> bool {
  return annotations_enabled(context, context.keyword());
}

// The set of property names that this schema declares as required at this
// level
inline auto required_properties(const KeywordContext &context)
    -> ValueStringSet {
  using Known = sourcemeta::core::SchemaVocabularies::Known;
  const auto imports_validation_vocabulary{
      context.vocabularies().contains(Known::JSON_SCHEMA_DRAFT_4) ||
      context.vocabularies().contains(Known::JSON_SCHEMA_DRAFT_6) ||
      context.vocabularies().contains(Known::JSON_SCHEMA_DRAFT_7) ||
      context.vocabularies().contains(Known::JSON_SCHEMA_2019_09_VALIDATION) ||
      context.vocabularies().contains(Known::JSON_SCHEMA_2020_12_VALIDATION) ||
      context.vocabularies().contains(Known::OPENAPI_3_0_BASE)};

  ValueStringSet result;

  if (imports_validation_vocabulary && context.schema().is_object() &&
      context.schema().defines("required") &&
      context.schema().at("required").is_array()) {
    for (const auto &entry : context.schema().at("required").as_array()) {
      if (entry.is_string()) {
        result.insert(entry.to_string());
      }
    }

    return result;
  }

  if (is_draft3(context.vocabularies()) && context.schema().is_object() &&
      context.schema().defines("properties") &&
      context.schema().at("properties").is_object()) {
    for (const auto &entry : context.schema().at("properties").as_object()) {
      // In Draft 3, keywords sibling to `$ref` are never evaluated, so a
      // `required` flag next to a `$ref` does not make the property mandatory
      if (entry.second.is_object() && entry.second.defines("required") &&
          !entry.second.defines("$ref") &&
          entry.second.at("required").is_boolean() &&
          entry.second.at("required").to_boolean()) {
        result.insert(entry.first);
      }
    }
  }

  return result;
}

// A disjunction branch is usually ruled out by a single discriminating
// property, as a tagged union does, yet the branch spends its first
// instructions on the wider checks that the tag makes irrelevant. Running the
// tag comparison first lets every branch but the matching one fail on one
// property lookup. The comparison is only sound ahead of the check that proves
// the property is there, so it becomes the variant that holds vacuously when
// the property is missing, leaving that proof to the check it now precedes
inline auto hoist_discriminator(Instructions &branch) -> void {
  ValueStringSet guaranteed;
  for (const auto &step : branch) {
    if (!step.relative_instance_location.empty()) {
      continue;
    }

    if (step.type == InstructionIndex::AssertionDefinesStrict) {
      const auto &property{std::get<ValueProperty>(step.value)};
      guaranteed.insert(property.first, property.second);
    } else if (step.type == InstructionIndex::AssertionDefinesAllStrict) {
      for (const auto &property : std::get<ValueStringSet>(step.value)) {
        guaranteed.insert(property.first, property.second);
      }
    }
  }

  if (guaranteed.empty()) {
    return;
  }

  const auto match{std::find_if(
      branch.begin(), branch.end(), [&guaranteed](const auto &step) -> bool {
        return (step.type == InstructionIndex::AssertionEqual ||
                step.type == InstructionIndex::AssertionEqualsAnyStringHash) &&
               step.relative_instance_location.size() == 1 &&
               step.relative_instance_location.back().is_property() &&
               guaranteed.contains(
                   step.relative_instance_location.back().to_property());
      })};

  if (match == branch.cend() || match == branch.cbegin()) {
    return;
  }

  match->type = match->type == InstructionIndex::AssertionEqual
                    ? InstructionIndex::AssertionPropertyEqual
                    : InstructionIndex::AssertionPropertyEqualsAnyStringHash;
  std::rotate(branch.begin(), match, std::next(match));
}

} // namespace sourcemeta::blaze

#endif
