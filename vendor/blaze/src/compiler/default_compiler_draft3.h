#ifndef SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT3_H_
#define SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT3_H_

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>

#include <sourcemeta/core/regex.h>

#include <algorithm> // std::sort, std::ranges::any_of, std::ranges::all_of, std::find_if, std::ranges::none_of
#include <cassert>   // assert
#include <cstdint>   // std::int32_t, std::int64_t
#include <initializer_list> // std::initializer_list
#include <limits>           // std::numeric_limits
#include <optional>         // std::optional, std::nullopt
#include <set>              // std::set
#include <string_view>      // std::string_view
#include <utility>          // std::move, std::to_underlying

#include "compile_helpers.h"
#include "upstream.h"

namespace sourcemeta::blaze {

// Draft 3 spells the absence of a type restriction as a type of its own, so a
// schema that declares `any` narrows nothing and every keyword it carries
// still applies. Only a type that rules the keyword out lets us skip it
static auto
narrows_type_away(const sourcemeta::core::JSON &schema,
                  const std::initializer_list<std::string_view> types) -> bool {
  const auto *value{schema.try_at("type")};
  if (value == nullptr || !value->is_string()) {
    return false;
  }

  const auto &name{value->to_string()};
  if (name == "any") {
    return false;
  }

  for (const auto candidate : types) {
    if (candidate == name) {
      return false;
    }
  }

  return true;
}

static auto parse_regex(const std::string &pattern,
                        const KeywordContext &context,
                        const sourcemeta::core::WeakPointer &schema_location)
    -> sourcemeta::core::Regex {
  const auto result{sourcemeta::core::to_regex(pattern)};
  if (!result.has_value()) [[unlikely]] {
    throw CompilerInvalidRegexError(context.base(),
                                    absolute_schema_location(context.global,
                                                             context.base(),
                                                             schema_location),
                                    pattern);
  }

  return result.value();
}

static auto relative_schema_location_size(const KeywordContext &context,
                                          const Instruction &step)
    -> std::size_t {
  return context.global.extra[step.extra_index].relative_schema_location.size();
}

static auto defines_direct_enumeration(const Instructions &steps)
    -> std::optional<std::size_t> {
  const auto iterator{std::ranges::find_if(steps, [](const auto &step) -> auto {
    return step.type == InstructionIndex::AssertionEqual ||
           step.type == InstructionIndex::AssertionEqualsAny;
  })};

  if (iterator == steps.cend()) {
    return std::nullopt;
  }

  return std::distance(steps.cbegin(), iterator);
}

static auto is_inside_disjunctor(const sourcemeta::core::WeakPointer &pointer)
    -> bool {
  return pointer.size() > 2 && pointer.at(pointer.size() - 2).is_index() &&
         pointer.at(pointer.size() - 3).is_property() &&
         (pointer.at(pointer.size() - 3).to_property() == "oneOf" ||
          pointer.at(pointer.size() - 3).to_property() == "anyOf");
}

// A subschema that `narrows_type_away` drops is never compiled, so neither
// the refusal at the end of the dispatch table nor the keyword shape checks
// ever reach its members. The OpenAPI 3.0 Schema Object is the only dialect
// here whose documents those checks judge as a whole, reachable or not, so
// compile the subschema the way a reachable one compiles and discard what it
// yields: the checks are the point rather than the instructions
static inline auto
compile_pruned_subschema(const KeywordContext &context,
                         const sourcemeta::core::WeakPointer &suffix) -> void {
  if (!is_openapi_3_0(context.vocabularies())) {
    return;
  }

  compile(context.relative(), suffix, suffix);
}

// A number too large to hold as a 64-bit integer is kept as a decimal, where
// whether it is an integer stays a question about how the literal was written
// rather than about how it came to be stored
auto is_dialect_integer(
    const sourcemeta::core::JSON &value,
    const sourcemeta::core::SchemaVocabularies &vocabularies) -> bool {
  if (!value.is_integral()) {
    return false;
  }

  return sourcemeta::core::schema_integral_reals_are_integers(vocabularies) ||
         value.is_integer() ||
         (value.is_decimal() && value.to_decimal().is_integer());
}

// OpenAPI 3.0 bans `type: "null"` and offers `nullable` in its place, which
// widens a sibling `type` and nothing else. It takes effect only where `type`
// is present, so on its own it constrains nothing, and it does not relax any
// other keyword, so a null instance still has to satisfy every sibling.
// The vocabulary has to be in play, or a member by that name in a dialect that
// gives it no meaning would be read as author data one moment and as a keyword
// the next
auto is_nullable(const sourcemeta::core::SchemaVocabularies &vocabularies,
                 const sourcemeta::core::JSON &schema) -> bool {
  return is_openapi_3_0(vocabularies) && schema.is_object() &&
         schema.defines("nullable") && schema.at("nullable").is_boolean() &&
         schema.at("nullable").to_boolean();
}

// Whether a sibling `type` names this type and may be taken as a guarantee
// about the instance. A scalar `type` normally is one, which lets the compiler
// specialise on it, either by folding another keyword's bound into the type
// assertion or by emitting an instruction that assumes the instance's shape.
// `nullable` widens it to also admit null, so there it guarantees nothing and
// every such specialisation has to stand down
auto assume_type(const sourcemeta::core::SchemaVocabularies &vocabularies,
                 const sourcemeta::core::JSON &schema,
                 const std::string_view name) -> bool {
  return schema.is_object() && schema.defines("type") &&
         schema.at("type").is_string() &&
         schema.at("type").to_string() == name &&
         !is_nullable(vocabularies, schema);
}

// Draft 4 onwards give `allOf`, `anyOf` and `oneOf` a `minItems` of one, so an
// empty array is malformed there. The OpenAPI 3.0 Schema Object constrains
// only the member type, leaving an empty one well-formed, and an empty
// conjunction or disjunction has a settled meaning of its own
auto in_place_applicators_may_be_empty(
    const sourcemeta::core::SchemaVocabularies &vocabularies) -> bool {
  return is_openapi_3_0(vocabularies);
}

// Draft 3 describes `type` and `disallow` as a type name or an array of unique
// entries, each of which is a type name or a subschema
auto is_draft3_type_union(const sourcemeta::core::JSON &value) -> bool {
  return value.is_array() && value.unique() &&
         std::ranges::all_of(value.as_array(), [](const auto &entry) -> bool {
           return entry.is_string() || entry.is_object();
         });
}

static auto json_array_to_string_set(const sourcemeta::core::JSON &document)
    -> ValueStringSet {
  ValueStringSet result;
  for (const auto &value : document.as_array()) {
    if (!value.is_string()) {
      continue;
    }

    result.insert(value.to_string());
  }

  return result;
}

static auto is_closed_properties_required(const sourcemeta::core::JSON &schema,
                                          const ValueStringSet &required)
    -> bool {
  return !schema.defines("patternProperties") &&
         schema.defines("additionalProperties") &&
         schema.at("additionalProperties").is_boolean() &&
         !schema.at("additionalProperties").to_boolean() &&
         schema.defines("properties") && schema.at("properties").is_object() &&
         schema.at("properties").size() == required.size() &&
         std::ranges::all_of(required, [&schema](const auto &property) -> auto {
           return schema.at("properties")
               .defines(property.first, property.second);
         });
}

static auto compile_properties(const KeywordContext &context,
                               const Instructions &)
    -> std::vector<std::pair<std::string, Instructions>> {
  // The keywords that fuse with `properties` reach it from the side, past the
  // point where an overridden keyword is dropped. A `properties` that `$ref`
  // overrides enforces nothing, and nothing under it is framed, so there is
  // neither anything to compile here nor anything to fuse with
  if (ref_overrides_sibling_keywords(context)) {
    return {};
  }

  std::vector<std::pair<std::string, Instructions>> properties;
  for (const auto &entry : context.schema().at("properties").as_object()) {
    properties.emplace_back(entry.first,
                            compile(context, make_weak_pointer(entry.first),
                                    make_weak_pointer(entry.first)));
  }

  // In many cases, `properties` have some subschemas that are small
  // and some subschemas that are large. To attempt to improve performance,
  // we prefer to evaluate smaller subschemas first, in the hope of failing
  // earlier without spending a lot of time on other subschemas
  if (context.global.tweaks.properties_reorder) {
    std::ranges::sort(
        properties, [&context](const auto &left, const auto &right) -> auto {
          const auto left_size{recursive_template_size(left.second)};
          const auto right_size{recursive_template_size(right.second)};
          if (left_size == right_size) {
            const auto left_direct_enumeration{
                defines_direct_enumeration(left.second)};
            const auto right_direct_enumeration{
                defines_direct_enumeration(right.second)};

            // Enumerations always take precedence
            if (left_direct_enumeration.has_value() &&
                right_direct_enumeration.has_value()) {
              // If both options have a direct enumeration, we choose
              // the one with the shorter relative schema location
              return relative_schema_location_size(
                         context,
                         left.second.at(left_direct_enumeration.value())) <
                     relative_schema_location_size(
                         context,
                         right.second.at(right_direct_enumeration.value()));
            }
            if (left_direct_enumeration.has_value()) {
              return true;
            }
            if (right_direct_enumeration.has_value()) {
              return false;
            }

            return left.first < right.first;
          }
          return left_size < right_size;
        });
  }

  return properties;
}

static auto to_string_hashes(
    std::vector<std::pair<ValueString, ValueStringSet::hash_type>> &hashes)
    -> ValueStringHashes {
  assert(!hashes.empty());
  std::ranges::sort(hashes, [](const auto &left, const auto &right) -> auto {
    return left.first.size() < right.first.size();
  });

  ValueStringHashes result;
  // The idea with the table of contents is as follows: each index
  // marks the starting and end positions for a string where the size
  // is equal to the index.
  result.second.resize(hashes.back().first.size() + 1, std::make_pair(0, 0));
  // TODO(C++23): Use std::views::enumerate when available in libc++
  for (std::size_t index = 0; index < hashes.size(); index++) {
    result.first.emplace_back(hashes[index].second, hashes[index].first);
    const auto string_size{hashes[index].first.size()};
    // We leave index 0 to represent the empty string
    const auto position{index + 1};
    const auto lower_bound{
        result.second[string_size].first == 0
            ? position
            : std::min(result.second[string_size].first, position)};
    const auto upper_bound{
        result.second[string_size].second == 0
            ? position
            : std::max(result.second[string_size].second, position)};
    assert(lower_bound <= upper_bound);
    assert(lower_bound > 0 && upper_bound > 0);
    assert(string_size < result.second.size());
    result.second[string_size] = std::make_pair(lower_bound, upper_bound);
  }

  assert(result.second.size() == hashes.back().first.size() + 1);
  return result;
}

static auto to_named_indexes(
    std::vector<std::pair<ValueString, ValueUnsignedInteger>> &names)
    -> ValueNamedIndexes {
  assert(!names.empty());
  std::ranges::sort(names, [](const auto &left, const auto &right) -> auto {
    return left.first.size() < right.first.size();
  });

  // The table of contents costs a load of its own before a lookup can compare
  // anything, which only pays off once there are enough names that scanning
  // them all is worse. Below that, the entries are left without one and the
  // evaluator scans them, as it did before the table of contents existed
  constexpr std::size_t MINIMUM_NAMES_FOR_TABLE_OF_CONTENTS{12};

  ValueNamedIndexes result;
  // The table of contents works as in `to_string_hashes`: each index marks the
  // starting and end positions of the names whose size is equal to the index
  if (names.size() > MINIMUM_NAMES_FOR_TABLE_OF_CONTENTS) {
    result.second.resize(names.back().first.size() + 1, std::make_pair(0, 0));
  }

  result.first.reserve(names.size());
  // TODO(C++23): Use std::views::enumerate when available in libc++
  for (std::size_t index = 0; index < names.size(); index++) {
    const auto &name{names[index].first};
    result.first.emplace_back(sourcemeta::core::JSON::Object::hash(name),
                              names[index].second, name);
    if (result.second.empty()) {
      continue;
    }

    // We leave position 0 to represent the absence of a size
    const auto position{index + 1};
    assert(name.size() < result.second.size());
    auto &bounds{result.second[name.size()]};
    bounds.first =
        (bounds.first == 0) ? position : std::min(bounds.first, position);
    bounds.second =
        (bounds.second == 0) ? position : std::max(bounds.second, position);
    assert(bounds.first <= bounds.second);
  }

  return result;
}

auto compile_required_assertions(const KeywordContext &context,
                                 const Instructions &current,
                                 ValueStringSet properties_set)
    -> Instructions {
  if (narrows_type_away(context.schema(), {"object"})) {
    return {};
  }

  const auto assume_object{
      assume_type(context.vocabularies(), context.schema(), "object")};

  if (properties_set.empty()) {
    return {};
  }
  if (properties_set.size() > 1) {
    if (is_closed_properties_required(context.schema(), properties_set)) {
      if (context.mode() == Mode::FastValidation && assume_object) {
        static const std::string PROPERTIES_KEYWORD{"properties"};
        // `SchemaContext::relative_pointer` is a reference, so the concatenated
        // pointer must outlive `new_schema_context`
        const auto properties_pointer{
            context.local.relative_pointer.initial().concat(
                make_weak_pointer(PROPERTIES_KEYWORD))};
        const SchemaContext new_schema_context{
            .relative_pointer = properties_pointer,
            .schema = context.schema(),
            .vocabularies = context.vocabularies(),
            .base = context.base(),
            .is_property_name = context.local.is_property_name};
        const KeywordContext properties_context{
            .global = context.global,
            .local = new_schema_context,
            .target = {.keyword = KEYWORD_PROPERTIES,
                       .base_schema_location =
                           sourcemeta::core::EMPTY_WEAK_POINTER,
                       .base_instance_location =
                           sourcemeta::core::EMPTY_WEAK_POINTER}};
        auto properties{compile_properties(properties_context, current)};
        if (std::ranges::all_of(properties, [](const auto &property) -> auto {
              return property.second.size() == 1 &&
                     property.second.front().type ==
                         InstructionIndex::AssertionTypeStrict;
            })) {
          std::set<ValueType> types;
          for (const auto &property : properties) {
            types.insert(std::get<ValueType>(property.second.front().value));
          }

          // `properties` only collapses into the fused form that also
          // enforces these requirements when it is neither emitting
          // annotations nor tracking evaluation, so this must mirror both of
          // its gates. Note that we must ask that of `properties` rather than
          // of this keyword, as evaluation tracking follows the keywords an
          // `unevaluatedProperties` depends on, which `required` is not one of
          if (types.size() == 1 &&
              !annotations_enabled(context, KEYWORD_PROPERTIES) &&
              !requires_evaluation(properties_context)) {
            // Handled in `properties`
            return {};
          }
        }

        sourcemeta::core::PropertyHashJSON<ValueString> hasher;
        if (context.mode() == Mode::FastValidation &&
            properties_set.size() == 3 &&
            std::ranges::all_of(properties_set,
                                [&hasher](const auto &property) -> auto {
                                  return hasher.is_perfect(property.second);
                                })) {
          std::vector<std::pair<ValueString, ValueStringSet::hash_type>> hashes;
          for (const auto &property : properties_set) {
            hashes.emplace_back(property.first, property.second);
          }

          return {make(InstructionIndex::AssertionDefinesExactlyStrictHash3,
                       context, to_string_hashes(hashes))};
        }

        return {make(InstructionIndex::AssertionDefinesExactlyStrict, context,
                     std::move(properties_set))};
      }
      return {make(InstructionIndex::AssertionDefinesExactly, context,
                   std::move(properties_set))};
    }
    if (assume_object) {
      return {make(InstructionIndex::AssertionDefinesAllStrict, context,
                   std::move(properties_set))};
    }
    return {make(InstructionIndex::AssertionDefinesAll, context,
                 std::move(properties_set))};
  }
  if (assume_object) {
    return {make(InstructionIndex::AssertionDefinesStrict, context,
                 make_property(properties_set.begin()->first))};
  }
  return {make(InstructionIndex::AssertionDefines, context,
               make_property(properties_set.begin()->first))};
}

auto compiler_draft3_core_ref(const KeywordContext &context,
                              const Instructions &) -> Instructions {
  const auto entry_pointer{absolute_schema_pointer(context)};
  const auto type{sourcemeta::core::SchemaReferenceType::Static};
  const auto reference{context.global.frame.reference(type, entry_pointer)};
  if (!reference.has_value()) [[unlikely]] {
    throw sourcemeta::core::SchemaReferenceError(
        "Could not resolve schema reference", context.value().to_string(),
        to_pointer(entry_pointer));
  }

  const auto key{std::make_tuple(type,
                                 std::string_view{reference->get().destination},
                                 context.local.is_property_name)};
  assert(context.global.targets.contains(key));
  return {make(InstructionIndex::ControlJump, context,
               ValueUnsignedInteger{context.global.targets.at(key).first})};
}

// There are two ways to compile `properties` depending on whether
// most of the properties are marked as required using `required`
// or whether most of the properties are optional. Each shines
// in the corresponding case.
auto properties_as_loop(const KeywordContext &context,
                        const sourcemeta::core::JSON &properties) -> bool {
  if (context.global.tweaks.properties_always_unroll) {
    return false;
  }

  using Known = sourcemeta::core::SchemaVocabularies::Known;
  const auto size{properties.size()};
  const auto imports_validation_vocabulary =
      context.vocabularies().contains(Known::JSON_SCHEMA_DRAFT_4) ||
      context.vocabularies().contains(Known::JSON_SCHEMA_DRAFT_6) ||
      context.vocabularies().contains(Known::JSON_SCHEMA_DRAFT_7) ||
      context.vocabularies().contains(Known::JSON_SCHEMA_2019_09_VALIDATION) ||
      context.vocabularies().contains(Known::JSON_SCHEMA_2020_12_VALIDATION) ||
      context.vocabularies().contains(Known::OPENAPI_3_0_BASE);
  // `const` is deliberately absent from the OpenAPI 3.0 dialect below, which
  // has `enum` but no `const` of its own
  const auto imports_const =
      context.vocabularies().contains(Known::JSON_SCHEMA_DRAFT_6) ||
      context.vocabularies().contains(Known::JSON_SCHEMA_DRAFT_7) ||
      context.vocabularies().contains(Known::JSON_SCHEMA_2019_09_VALIDATION) ||
      context.vocabularies().contains(Known::JSON_SCHEMA_2020_12_VALIDATION);
  ValueStringSet required;
  for (const auto &entry : required_properties(context)) {
    // Only count the required property if its indeed in "properties"
    if (properties.defines(entry.first)) {
      required.insert(entry.first);
    }
  }

  const auto current_entry_pointer{absolute_schema_pointer(context)};
  const auto inside_disjunctor{
      is_inside_disjunctor(context.local.relative_pointer) ||
      // Check if any reference from `anyOf` or `oneOf` points to us
      context.global.frame.any_reference(
          [&context, &current_entry_pointer](
              const sourcemeta::core::SchemaReferenceType,
              const sourcemeta::core::WeakPointer &origin,
              const sourcemeta::core::SchemaFrame::Reference &reference)
              -> bool {
            const auto destination{context.global.frame.location(
                sourcemeta::core::SchemaReferenceType::Static,
                reference.destination)};
            if (!destination.has_value()) {
              return false;
            }

            return is_inside_disjunctor(origin) &&
                   current_entry_pointer.initial() ==
                       destination.value().get().pointer;
          })};

  if (!inside_disjunctor && context.schema().defines("additionalProperties") &&
      context.schema().at("additionalProperties").is_boolean() &&
      !context.schema().at("additionalProperties").to_boolean() &&
      // If all properties are required, we should still unroll
      required.size() < size) {
    return true;
  }

  return
      // This strategy only makes sense if most of the properties are "optional"
      required.size() <= (size / 4) &&
      // If `properties` only defines a relatively small amount of properties,
      // then its probably still faster to unroll
      size > 5 &&
      // Always unroll inside `oneOf` or `anyOf`, to have a
      // better chance at quickly short-circuiting
      (!inside_disjunctor ||
       std::ranges::none_of(
           properties.as_object(), [&](const auto &pair) -> auto {
             return pair.second.is_object() &&
                    ((imports_validation_vocabulary &&
                      pair.second.defines("enum")) ||
                     (imports_const && pair.second.defines("const")));
           }));
}

auto draft3_any_type_instructions(const KeywordContext &context)
    -> Instructions {
  if (context.mode() != Mode::Exhaustive) {
    return {};
  }

  using sourcemeta::core::JSON;
  ValueTypes types{};
  types.set(std::to_underlying(JSON::Type::Null));
  types.set(std::to_underlying(JSON::Type::Boolean));
  types.set(std::to_underlying(JSON::Type::Object));
  types.set(std::to_underlying(JSON::Type::Array));
  types.set(std::to_underlying(JSON::Type::Integer));
  types.set(std::to_underlying(JSON::Type::Real));
  types.set(std::to_underlying(JSON::Type::Decimal));
  types.set(std::to_underlying(JSON::Type::String));
  return {make(InstructionIndex::AssertionTypeStrictAny, context, types)};
}

// The instruction that asserts a single type name, or nothing at all for a
// name this does not know, which constrains nothing
auto make_type_assertion(const KeywordContext &context,
                         const std::string_view name)
    -> std::optional<Instruction> {
  using sourcemeta::core::JSON;

  if (name == "number") {
    ValueTypes types{};
    types.set(std::to_underlying(JSON::Type::Real));
    types.set(std::to_underlying(JSON::Type::Integer));
    types.set(std::to_underlying(JSON::Type::Decimal));
    return make(InstructionIndex::AssertionTypeStrictAny, context, types);
  }

  // A dialect that reads a number written with a fractional part as an integer
  // needs the check that accepts one
  if (name == "integer") {
    return make(sourcemeta::core::schema_integral_reals_are_integers(
                    context.vocabularies())
                    ? InstructionIndex::AssertionType
                    : InstructionIndex::AssertionTypeStrict,
                context, JSON::Type::Integer);
  }

  JSON::Type type{};
  if (name == "null") {
    type = JSON::Type::Null;
  } else if (name == "boolean") {
    type = JSON::Type::Boolean;
  } else if (name == "object") {
    type = JSON::Type::Object;
  } else if (name == "array") {
    type = JSON::Type::Array;
  } else if (name == "string") {
    type = JSON::Type::String;
  } else {
    return std::nullopt;
  }

  return make(InstructionIndex::AssertionTypeStrict, context, type);
}

// A pair of sibling size bounds folds into the type assertion itself, which
// then settles the type and the size in one instruction. Bounds that cross
// leave nothing for an instance to satisfy
auto fuse_type_size_bound(const KeywordContext &context,
                          const std::size_t minimum,
                          const std::optional<std::size_t> maximum,
                          const InstructionIndex upper,
                          const InstructionIndex bounded)
    -> std::optional<Instructions> {
  if (context.mode() != Mode::FastValidation) {
    return std::nullopt;
  }

  if (maximum.has_value() && minimum > maximum.value()) {
    return Instructions{
        make(InstructionIndex::AssertionFail, context, ValueNone{})};
  }

  if (maximum.has_value() && minimum == 0) {
    return Instructions{
        make(upper, context, ValueUnsignedInteger{maximum.value()})};
  }

  if (minimum > 0 || maximum.has_value()) {
    return Instructions{
        make(bounded, context, ValueRange{minimum, maximum, false})};
  }

  return std::nullopt;
}

// Whether a value is of the type this name stands for. Note that `integer`
// asks whether the value is integral rather than whether it is held as an
// integer, as equality reads a number by its value, so an option written with
// a fractional part still matches an integer instance
auto value_matches_type_name(const sourcemeta::core::JSON &value,
                             const std::string_view name) -> bool {
  if (name == "null") {
    return value.is_null();
  }
  if (name == "boolean") {
    return value.is_boolean();
  }
  if (name == "object") {
    return value.is_object();
  }
  if (name == "array") {
    return value.is_array();
  }
  if (name == "number") {
    return value.is_number();
  }
  if (name == "integer") {
    return value.is_integral();
  }
  if (name == "string") {
    return value.is_string();
  }

  return false;
}

// A type assertion says nothing that the options of a sibling `enum` do not
// already say, as every one of them is of the type anyway. An `enum` that
// stands empty admits nothing at all, which its own keyword turns into a
// failure, so the type assertion is just as spare there
auto type_covered_by_enum(const KeywordContext &context,
                          const std::string_view name) -> bool {
  if (context.mode() != Mode::FastValidation) {
    return false;
  }

  const auto *options{context.schema().try_at("enum")};
  return options != nullptr && options->is_array() &&
         std::ranges::all_of(options->as_array(),
                             [name](const auto &option) -> bool {
                               return value_matches_type_name(option, name);
                             });
}

// The same, for the one option that `const` spells on its own. Only a dialect
// that defines the keyword may be asked, as a member by that name elsewhere is
// author data rather than a constraint
auto type_covered_by_const(const KeywordContext &context,
                           const std::string_view name) -> bool {
  if (context.mode() != Mode::FastValidation) {
    return false;
  }

  const auto *value{context.schema().try_at("const")};
  return value != nullptr && value_matches_type_name(*value, name);
}

auto draft3_set_type_bits(const std::string &name, ValueTypes &types) -> bool {
  using sourcemeta::core::JSON;
  if (name == "null") {
    types.set(std::to_underlying(JSON::Type::Null));
  } else if (name == "boolean") {
    types.set(std::to_underlying(JSON::Type::Boolean));
  } else if (name == "object") {
    types.set(std::to_underlying(JSON::Type::Object));
  } else if (name == "array") {
    types.set(std::to_underlying(JSON::Type::Array));
  } else if (name == "integer") {
    types.set(std::to_underlying(JSON::Type::Integer));
  } else if (name == "string") {
    types.set(std::to_underlying(JSON::Type::String));
  } else if (name == "number") {
    types.set(std::to_underlying(JSON::Type::Real));
    types.set(std::to_underlying(JSON::Type::Integer));
    types.set(std::to_underlying(JSON::Type::Decimal));
  } else {
    return false;
  }
  return true;
}

auto is_integer_type_check(const Instruction &instruction) -> bool {
  return (instruction.type == InstructionIndex::AssertionType ||
          instruction.type == InstructionIndex::AssertionTypeStrict) &&
         std::get<ValueType>(instruction.value) ==
             sourcemeta::core::JSON::Type::Integer;
}

auto has_strict_integer_type(const Instructions &children) -> bool {
  for (const auto &child : children) {
    if (child.type == InstructionIndex::AssertionTypeStrict &&
        std::get<ValueType>(child.value) ==
            sourcemeta::core::JSON::Type::Integer) {
      return true;
    }
  }

  return false;
}

auto is_integer_type_bounded_pattern(const Instructions &children) -> bool {
  if (children.size() != 3) {
    return false;
  }

  bool has_type{false};
  bool has_min{false};
  bool has_max{false};
  for (const auto &child : children) {
    if (is_integer_type_check(child)) {
      has_type = true;
    } else if (child.type == InstructionIndex::AssertionGreaterEqual &&
               std::get<ValueJSON>(child.value).is_integer()) {
      has_min = true;
    } else if (child.type == InstructionIndex::AssertionLessEqual &&
               std::get<ValueJSON>(child.value).is_integer()) {
      has_max = true;
    }
  }

  return has_type && has_min && has_max;
}

auto is_integer_type_lower_bound_pattern(const Instructions &children) -> bool {
  if (children.size() != 2) {
    return false;
  }

  bool has_type{false};
  bool has_min{false};
  for (const auto &child : children) {
    if (is_integer_type_check(child)) {
      has_type = true;
    } else if (child.type == InstructionIndex::AssertionGreaterEqual &&
               std::get<ValueJSON>(child.value).is_integer()) {
      has_min = true;
    }
  }

  return has_type && has_min;
}

auto extract_integer_lower_bound(const Instructions &children) -> std::int64_t {
  for (const auto &child : children) {
    if (child.type == InstructionIndex::AssertionGreaterEqual) {
      return std::get<ValueJSON>(child.value).to_integer();
    }
  }

  return 0;
}

auto extract_integer_bounds(const Instructions &children)
    -> ValueIntegerBounds {
  std::int64_t minimum{0};
  std::int64_t maximum{0};
  for (const auto &child : children) {
    if (child.type == InstructionIndex::AssertionGreaterEqual) {
      minimum = std::get<ValueJSON>(child.value).to_integer();
    } else if (child.type == InstructionIndex::AssertionLessEqual) {
      maximum = std::get<ValueJSON>(child.value).to_integer();
    }
  }

  return {minimum, maximum};
}

auto integer_lower_bound_extra_index(const Instructions &children)
    -> std::size_t {
  for (const auto &child : children) {
    if (child.type == InstructionIndex::AssertionGreaterEqual) {
      return child.extra_index;
    }
  }

  return 0;
}

auto compiler_draft3_applicator_properties_with_options(
    const KeywordContext &context, const Instructions &current,
    const bool annotate, const bool track_evaluation) -> Instructions {
  if (context.local.is_property_name) {
    return {};
  }

  if (!context.value().is_object()) {
    throw_keyword_error(context, EXPECTED_OBJECT);
  }

  if (context.value().empty()) {
    return {};
  }

  if (narrows_type_away(context.schema(), {"object"})) {
    for (const auto &entry : context.value().as_object()) {
      compile_pruned_subschema(context, make_weak_pointer(entry.first));
    }

    return {};
  }

  const bool emit_annotation{annotate && annotations_enabled(context)};

  if (properties_as_loop(context, context.value())) {
    std::vector<std::pair<ValueString, ValueUnsignedInteger>> names;
    Instructions children;
    std::size_t cursor = 0;

    for (auto &&[name, substeps] :
         compile_properties(context.relative(), current)) {
      names.emplace_back(name, cursor);

      if (track_evaluation) {
        substeps.push_back(make(InstructionIndex::ControlEvaluate,
                                context.relative(), ValuePointer{name}));
      }

      if (emit_annotation) {
        substeps.push_back(make(InstructionIndex::AnnotationEmitWrapped,
                                context.relative(),
                                sourcemeta::core::JSON{name}));
      }

      // Note that the evaluator completely ignores this wrapper anyway
      children.push_back(make(InstructionIndex::ControlGroup,
                              context.relative(), ValueNone{},
                              std::move(substeps)));
      cursor += 1;
    }

    if (context.mode() == Mode::FastValidation && !track_evaluation &&
        !context.schema().defines("patternProperties") &&
        context.schema().defines("additionalProperties") &&
        context.schema().at("additionalProperties").is_boolean() &&
        !context.schema().at("additionalProperties").to_boolean()) {
      return {make(InstructionIndex::LoopPropertiesMatchClosed, context,
                   to_named_indexes(names), std::move(children))};
    }

    return {make(InstructionIndex::LoopPropertiesMatch, context,
                 to_named_indexes(names), std::move(children))};
  }

  Instructions children;

  const auto effective_context{
      context.mode() == Mode::FastValidation ? context : context.relative()};

  const auto assume_object{
      assume_type(context.vocabularies(), context.schema(), "object")};

  ValueStringSet required{required_properties(context)};

  auto properties{compile_properties(effective_context, current)};

  // These fused forms collapse every property into a single instruction that
  // does not mark the properties it matches as evaluated, so under evaluation
  // tracking we must fall through to the form that emits the markers
  if (!emit_annotation && context.mode() == Mode::FastValidation &&
      !track_evaluation && !required.empty() &&
      context.schema().defines("additionalProperties") &&
      context.schema().at("additionalProperties").is_boolean() &&
      !context.schema().at("additionalProperties").to_boolean() &&
      required.size() == context.value().size() &&
      std::ranges::all_of(properties,
                          [&required](const auto &property) -> auto {
                            return required.contains(property.first);
                          })) {
    if (std::ranges::all_of(properties, [](const auto &property) -> auto {
          return property.second.size() == 1 &&
                 property.second.front().type ==
                     InstructionIndex::AssertionTypeStrict;
        })) {
      std::set<ValueType> types;
      for (const auto &property : properties) {
        types.insert(std::get<ValueType>(property.second.front().value));
      }

      if (types.size() == 1 && !context.schema().defines("patternProperties")) {
        if (assume_object) {
          if (is_closed_properties_required(context.schema(), required)) {
            sourcemeta::core::PropertyHashJSON<ValueString> hasher;
            std::vector<std::pair<ValueString, ValueStringSet::hash_type>>
                perfect_hashes;
            for (const auto &entry : required) {
              assert(required.contains(entry.first, entry.second));
              if (hasher.is_perfect(entry.second)) {
                perfect_hashes.emplace_back(entry.first, entry.second);
              }
            }

            if (perfect_hashes.size() == required.size()) {
              return {
                  make(InstructionIndex::LoopPropertiesExactlyTypeStrictHash,
                       context,
                       ValueTypedHashes{*types.cbegin(),
                                        to_string_hashes(perfect_hashes)})};
            }

            return {make(
                InstructionIndex::LoopPropertiesExactlyTypeStrict, context,
                ValueTypedProperties{*types.cbegin(), std::move(required)})};
          }
        }

        return {make(InstructionIndex::LoopPropertiesTypeStrict, context,
                     *types.cbegin())};
      }
    }

    if (std::ranges::all_of(properties, [](const auto &property) -> auto {
          return property.second.size() == 1 &&
                 property.second.front().type ==
                     InstructionIndex::AssertionType;
        })) {
      std::set<ValueType> types;
      for (const auto &property : properties) {
        types.insert(std::get<ValueType>(property.second.front().value));
      }

      if (types.size() == 1) {
        return {make(InstructionIndex::LoopPropertiesType, context,
                     *types.cbegin())};
      }
    }
  }

  auto attempt_object_fusion{context.mode() == Mode::FastValidation &&
                             !emit_annotation && !track_evaluation &&
                             assume_object};
  if (attempt_object_fusion) {
    for (const auto &entry : context.schema().as_object()) {
      const auto &keyword{entry.first};
      if (keyword == "type" || keyword == "required" ||
          keyword == context.keyword()) {
        continue;
      }

      if (keyword == "additionalProperties" && entry.second.is_boolean() &&
          entry.second.to_boolean()) {
        continue;
      }

      const auto &keyword_type{
          context.global.walker(keyword, context.vocabularies()).type};
      using enum sourcemeta::core::SchemaKeywordType;
      if (keyword_type == Assertion || keyword_type == Annotation ||
          keyword_type == Unknown || keyword_type == Comment ||
          keyword_type == Other || keyword_type == LocationMembers) {
        continue;
      }

      attempt_object_fusion = false;
      break;
    }
  }
  ValueObjectProperties fusion_entries;
  Instructions fusion_children;
  bool fusion_possible{attempt_object_fusion};

  for (auto &&[name, substeps] : properties) {
    if (emit_annotation) {
      substeps.push_back(make(InstructionIndex::AnnotationEmitWrapped,
                              effective_context, sourcemeta::core::JSON{name}));
    }

    // Optimize `properties` where its subschemas just include a type check

    if (context.mode() == Mode::FastValidation && track_evaluation &&
        substeps.size() == 1 &&
        substeps.front().type == InstructionIndex::AssertionTypeStrict) {
      children.push_back(rephrase(
          context, InstructionIndex::AssertionPropertyTypeStrictEvaluate,
          substeps.front()));
    } else if (context.mode() == Mode::FastValidation && track_evaluation &&
               substeps.size() == 1 &&
               substeps.front().type == InstructionIndex::AssertionType) {
      children.push_back(
          rephrase(context, InstructionIndex::AssertionPropertyTypeEvaluate,
                   substeps.front()));
    } else if (context.mode() == Mode::FastValidation && track_evaluation &&
               substeps.size() == 1 &&
               substeps.front().type ==
                   InstructionIndex::AssertionTypeStrictAny) {
      children.push_back(rephrase(
          context, InstructionIndex::AssertionPropertyTypeStrictAnyEvaluate,
          substeps.front()));

      // NOLINTBEGIN(bugprone-branch-clone)
    } else if (!fusion_possible && context.mode() == Mode::FastValidation &&
               substeps.size() == 1 &&
               substeps.front().type ==
                   InstructionIndex::AssertionPropertyTypeStrict) {
      children.push_back(
          unroll(context, substeps.front(),
                 effective_context.target.base_instance_location));
    } else if (!fusion_possible && context.mode() == Mode::FastValidation &&
               substeps.size() == 1 &&
               substeps.front().type ==
                   InstructionIndex::AssertionPropertyType) {
      children.push_back(
          unroll(context, substeps.front(),
                 effective_context.target.base_instance_location));
    } else if (!fusion_possible && context.mode() == Mode::FastValidation &&
               substeps.size() == 1 &&
               substeps.front().type ==
                   InstructionIndex::AssertionPropertyTypeStrictAny) {
      children.push_back(
          unroll(context, substeps.front(),
                 effective_context.target.base_instance_location));
      // NOLINTEND(bugprone-branch-clone)

    } else {
      if (track_evaluation) {
        auto new_base_instance_location{
            effective_context.target.base_instance_location};
        new_base_instance_location.push_back({name});
        const KeywordContext evaluate_context{
            .global = effective_context.global,
            .local = effective_context.local,
            .target = {.keyword = effective_context.keyword(),
                       .base_schema_location =
                           effective_context.target.base_schema_location,
                       .base_instance_location = new_base_instance_location}};
        substeps.push_back(
            make(InstructionIndex::Evaluate, evaluate_context, ValueNone{}));
      }

      if (context.mode() == Mode::FastValidation && !substeps.empty()) {
        if (is_integer_type_bounded_pattern(substeps)) {
          auto bounds = extract_integer_bounds(substeps);
          const auto index =
              has_strict_integer_type(substeps)
                  ? InstructionIndex::AssertionTypeIntegerBoundedStrict
                  : InstructionIndex::AssertionTypeIntegerBounded;
          auto instance_location = substeps.front().relative_instance_location;
          const auto minimum_extra_index{
              integer_lower_bound_extra_index(substeps)};
          substeps.clear();
          auto fused = make(index, context.relative(), bounds);
          fused.relative_instance_location = std::move(instance_location);
          fused.extra_index = minimum_extra_index;
          substeps.push_back(std::move(fused));
        } else if (is_integer_type_lower_bound_pattern(substeps)) {
          const auto minimum = extract_integer_lower_bound(substeps);
          const auto index =
              has_strict_integer_type(substeps)
                  ? InstructionIndex::AssertionTypeIntegerLowerBoundStrict
                  : InstructionIndex::AssertionTypeIntegerLowerBound;
          auto instance_location = substeps.front().relative_instance_location;
          const auto minimum_extra_index{
              integer_lower_bound_extra_index(substeps)};
          substeps.clear();
          auto fused =
              make(index, context.relative(), ValueIntegerBounds{minimum, 0});
          fused.relative_instance_location = std::move(instance_location);
          fused.extra_index = minimum_extra_index;
          substeps.push_back(std::move(fused));
        } else if (substeps.size() == 2) {
          bool has_items_bounded{false};
          bool has_array_type{false};
          std::size_t items_index{0};
          std::size_t array_index{0};
          for (std::size_t step_index = 0; step_index < 2; step_index++) {
            if (substeps[step_index].type ==
                InstructionIndex::LoopItemsIntegerBounded) {
              has_items_bounded = true;
              items_index = step_index;
            } else if (substeps[step_index].type ==
                       InstructionIndex::AssertionTypeArrayBounded) {
              has_array_type = true;
              array_index = step_index;
            }
          }

          if (has_items_bounded && has_array_type) {
            auto integer_bounds{
                std::get<ValueIntegerBounds>(substeps[items_index].value)};
            auto range{std::get<ValueRange>(substeps[array_index].value)};
            auto instance_location =
                substeps[items_index].relative_instance_location;
            Value fused_value{
                ValueIntegerBoundsWithSize{integer_bounds, std::move(range)}};
            substeps.clear();
            auto fused = make(InstructionIndex::LoopItemsIntegerBoundedSized,
                              effective_context, fused_value);
            fused.relative_instance_location = std::move(instance_location);
            substeps.push_back(std::move(fused));
          }
        }
      }

      // A fused check rejects a non-object and enforces the presence of the
      // properties it marks as required, but only those, and only at its own
      // instance location. This list is flattened and may also hold checks
      // merged in from elsewhere, as every `allOf` branch contributes to it,
      // so we may only drop what the fusion provably covers
      std::vector<
          std::pair<sourcemeta::core::Pointer, std::unordered_set<std::string>>>
          fusions;
      if (fusion_possible && substeps.size() >= 2) {
        for (const auto &step : substeps) {
          if (step.type != InstructionIndex::AssertionObjectPropertiesSimple) {
            continue;
          }

          auto match{
              std::ranges::find_if(fusions, [&step](const auto &entry) -> bool {
                return entry.first == step.relative_instance_location;
              })};
          if (match == fusions.end()) {
            fusions.emplace_back(step.relative_instance_location,
                                 std::unordered_set<std::string>{});
            match = std::prev(fusions.end());
          }

          for (const auto &entry :
               std::get<ValueObjectProperties>(step.value)) {
            if (std::get<2>(entry)) {
              match->second.insert(std::get<0>(entry));
            }
          }
        }
      }

      if (!fusions.empty()) {
        std::erase_if(substeps, [&fusions](const auto &step) -> auto {
          const auto fusion{
              std::ranges::find_if(fusions, [&step](const auto &entry) -> bool {
                return entry.first == step.relative_instance_location;
              })};
          if (fusion == fusions.cend()) {
            return false;
          }

          if (step.type == InstructionIndex::AssertionDefinesAllStrict ||
              step.type == InstructionIndex::AssertionDefinesAll) {
            const auto &value{std::get<ValueStringSet>(step.value)};
            return std::ranges::all_of(
                value, [&fusion](const auto &property) -> auto {
                  return fusion->second.contains(property.first);
                });
          }

          if ((step.type == InstructionIndex::AssertionTypeStrict ||
               step.type == InstructionIndex::AssertionType) &&
              std::get<ValueType>(step.value) ==
                  sourcemeta::core::JSON::Type::Object) {
            return true;
          }

          return false;
        });
      }

      // Fusion re-applies the single check to every property of the enclosing
      // loop with its instance location cleared, so a check whose meaning
      // depends on its own instance location cannot be relocated this way. A
      // jump into a separate target, or a check that navigates into or gates
      // on a nested location such as the one a `properties` conditional
      // produces, therefore falls back to unfused emission
      const bool fusable_single{
          substeps.size() == 1 &&
          substeps.front().type != InstructionIndex::ControlJump &&
          substeps.front().type != InstructionIndex::ControlDynamicAnchorJump &&
          substeps.front().type != InstructionIndex::ControlGroup &&
          substeps.front().type != InstructionIndex::ControlGroupWhenDefines &&
          substeps.front().type !=
              InstructionIndex::ControlGroupWhenDefinesDirect &&
          substeps.front().type != InstructionIndex::ControlGroupWhenType &&
          substeps.front().type != InstructionIndex::ControlEvaluate &&
          substeps.front().type != InstructionIndex::ControlFrame};
      if (fusion_possible && fusable_single) {
        const auto is_required{assume_object && required.contains(name)};
        auto prop{make_property(name)};
        auto fusion_child{substeps.front()};
        fusion_child.relative_instance_location = {};
        auto fusion_extra{context.global.extra[fusion_child.extra_index]};
        fusion_extra.relative_schema_location = {};
        fusion_child.extra_index = context.global.extra.size();
        context.global.extra.push_back(std::move(fusion_extra));

        fusion_entries.emplace_back(prop.first, prop.second, is_required);
        fusion_children.push_back(std::move(fusion_child));
      } else {
        fusion_possible = false;
      }

      if (!substeps.empty()) {
        // As a performance shortcut
        if (effective_context.target.base_instance_location.empty()) {
          if (assume_object && required.contains(name)) {
            for (auto &&step : substeps) {
              children.push_back(std::move(step));
            }
          } else {
            children.push_back(make(
                InstructionIndex::ControlGroupWhenDefinesDirect,
                effective_context, make_property(name), std::move(substeps)));
          }
        } else {
          children.push_back(make(InstructionIndex::ControlGroupWhenDefines,
                                  effective_context, make_property(name),
                                  std::move(substeps)));
        }
      }
    }
  }

  if (context.mode() == Mode::FastValidation) {
    if (fusion_possible && !fusion_entries.empty() &&
        !context.global.collects_annotations) {
      for (const auto &req : required) {
        const auto &req_name{req.first};
        bool already_tracked{false};
        for (const auto &entry : fusion_entries) {
          if (std::get<0>(entry) == req_name) {
            already_tracked = true;
            break;
          }
        }
        if (!already_tracked) {
          auto prop{make_property(req_name)};
          fusion_entries.emplace_back(prop.first, prop.second, true);
        }
      }

      if (fusion_entries.size() > 32) {
        return children;
      }

      return {make(InstructionIndex::AssertionObjectPropertiesSimple, context,
                   Value{std::move(fusion_entries)},
                   std::move(fusion_children))};
    }

    return children;
  }
  if (children.empty()) {
    return {};
  }
  return {make(InstructionIndex::LogicalWhenType, context,
               sourcemeta::core::JSON::Type::Object, std::move(children))};
}

auto compiler_draft3_applicator_properties(const KeywordContext &context,
                                           const Instructions &current)
    -> Instructions {
  auto property_instructions{compiler_draft3_applicator_properties_with_options(
      context, current, false, false)};

  if (!is_draft3(context.vocabularies())) {
    return property_instructions;
  }

  ValueStringSet required{required_properties(context)};
  if (required.empty()) {
    return property_instructions;
  }

  auto required_instructions{
      compile_required_assertions(context, current, std::move(required))};
  if (required_instructions.empty()) {
    return property_instructions;
  }

  Instructions result{std::move(required_instructions)};
  for (auto &&step : property_instructions) {
    result.push_back(std::move(step));
  }
  return result;
}

auto compiler_draft3_applicator_patternproperties_with_options(
    const KeywordContext &context, const bool annotate,
    const bool track_evaluation) -> Instructions {
  if (!context.value().is_object()) {
    throw_keyword_error(context, EXPECTED_OBJECT);
  }

  if (context.value().empty()) {
    return {};
  }

  if (narrows_type_away(context.schema(), {"object"})) {
    return {};
  }

  Instructions children;

  // To guarantee ordering
  std::vector<std::string> patterns;
  for (const auto &entry : context.value().as_object()) {
    patterns.push_back(entry.first);
  }

  std::ranges::sort(patterns);

  // For each regular expression and corresponding subschema in the object
  for (const auto &pattern : patterns) {
    auto substeps{compile(context.relative(), make_weak_pointer(pattern))};

    if (annotate && annotations_enabled(context)) {
      substeps.push_back(make(InstructionIndex::AnnotationBasenameToParent,
                              context.relative(), ValueNone{}));
    }

    if (track_evaluation) {
      substeps.push_back(make(InstructionIndex::ControlEvaluate,
                              context.relative(), ValuePointer{}));
    }

    if (context.mode() == Mode::FastValidation && !track_evaluation &&
        patterns.size() == 1 &&
        (!context.schema().defines("properties") ||
         (context.schema().at("properties").is_object() &&
          context.schema().at("properties").empty())) &&
        context.schema().defines("additionalProperties") &&
        context.schema().at("additionalProperties").is_boolean() &&
        !context.schema().at("additionalProperties").to_boolean()) {
      children.push_back(
          make(InstructionIndex::LoopPropertiesRegexClosed, context,
               ValueRegex{.first = parse_regex(pattern, context,
                                               context.local.relative_pointer),
                          .second = pattern},
               std::move(substeps)));

      // If the `patternProperties` subschema for the given pattern does
      // nothing, then we can avoid generating an entire loop for it
    } else if (!substeps.empty()) {
      const auto maybe_prefix{pattern_as_prefix(pattern)};
      if (maybe_prefix.has_value()) {
        children.push_back(make(InstructionIndex::LoopPropertiesStartsWith,
                                context, ValueString{maybe_prefix.value()},
                                std::move(substeps)));
      } else {
        children.push_back(make(
            InstructionIndex::LoopPropertiesRegex, context,
            ValueRegex{.first = parse_regex(pattern, context,
                                            context.local.relative_pointer),
                       .second = pattern},
            std::move(substeps)));
      }
    }
  }

  return children;
}

auto compiler_draft3_applicator_patternproperties(const KeywordContext &context,
                                                  const Instructions &)
    -> Instructions {
  return compiler_draft3_applicator_patternproperties_with_options(
      context, false, false);
}

// Determine whether the `properties` keyword on its own enforces a closed
// object (i.e. compiles to one of the `LoopPropertiesExactly*` forms). This
// happens when every property subschema reduces to a single strict type
// assertion of the same type. In that case `additionalProperties: false` is
// already enforced by `properties` and does not need to emit anything.
inline auto properties_enforce_closed_object(const KeywordContext &context)
    -> bool {
  const bool assume_object{
      assume_type(context.vocabularies(), context.schema(), "object")};
  if (!assume_object || !context.schema().defines("properties") ||
      !context.schema().at("properties").is_object()) {
    return false;
  }

  // `SchemaContext::relative_pointer` is a reference, so the concatenated
  // pointer must outlive `new_schema_context`
  const auto properties_pointer{context.local.relative_pointer.initial().concat(
      make_weak_pointer(KEYWORD_PROPERTIES))};
  const SchemaContext new_schema_context{.relative_pointer = properties_pointer,
                                         .schema = context.schema(),
                                         .vocabularies = context.vocabularies(),
                                         .base = context.base(),
                                         .is_property_name =
                                             context.local.is_property_name};
  const KeywordContext properties_context{
      .global = context.global,
      .local = new_schema_context,
      .target = {.keyword = KEYWORD_PROPERTIES,
                 .base_schema_location = sourcemeta::core::EMPTY_WEAK_POINTER,
                 .base_instance_location =
                     sourcemeta::core::EMPTY_WEAK_POINTER}};
  const auto properties{compile_properties(properties_context, {})};
  if (!std::ranges::all_of(properties, [](const auto &property) -> auto {
        return property.second.size() == 1 &&
               property.second.front().type ==
                   InstructionIndex::AssertionTypeStrict;
      })) {
    return false;
  }

  std::set<ValueType> types;
  for (const auto &property : properties) {
    types.insert(std::get<ValueType>(property.second.front().value));
  }

  return types.size() == 1;
}

auto compiler_draft3_applicator_additionalproperties_with_options(
    const KeywordContext &context, const bool annotate,
    const bool track_evaluation) -> Instructions {
  if (narrows_type_away(context.schema(), {"object"})) {
    compile_pruned_subschema(context, sourcemeta::core::EMPTY_WEAK_POINTER);
    return {};
  }

  Instructions children{compile(context.relative(),
                                sourcemeta::core::EMPTY_WEAK_POINTER,
                                sourcemeta::core::EMPTY_WEAK_POINTER)};

  if (annotate && annotations_enabled(context)) {
    children.push_back(make(InstructionIndex::AnnotationBasenameToParent,
                            context.relative(), ValueNone{}));
  }

  ValueStringSet filter_strings;
  ValueStrings filter_prefixes;
  std::vector<ValueRegex> filter_regexes;

  if (context.schema().defines("properties") &&
      context.schema().at("properties").is_object()) {
    for (const auto &entry : context.schema().at("properties").as_object()) {
      filter_strings.insert(entry.first);
    }
  }

  if (context.schema().defines("patternProperties") &&
      context.schema().at("patternProperties").is_object()) {
    for (const auto &entry :
         context.schema().at("patternProperties").as_object()) {
      const auto maybe_prefix{pattern_as_prefix(entry.first)};
      if (maybe_prefix.has_value()) {
        filter_prefixes.push_back(maybe_prefix.value());
      } else {
        static const std::string PATTERN_PROPERTIES_KEYWORD{
            "patternProperties"};
        filter_regexes.push_back(
            {.first = parse_regex(
                 entry.first, context,
                 context.local.relative_pointer.initial().concat(
                     make_weak_pointer(PATTERN_PROPERTIES_KEYWORD))),
             .second = entry.first});
      }
    }
  }

  // For performance, if a schema sets `additionalProperties: true` (or its
  // variants), we don't need to do anything
  if (!track_evaluation && children.empty()) {
    return {};
  }

  // The closed forms that the elisions below rely on are only emitted when
  // `additionalProperties` is the boolean `false`. Testing the compiled shape
  // alone is not enough, as other subschemas that reject everything, like an
  // empty `enum`, also reduce to an unconditional failure without closing
  // anything
  const auto rejects_with_boolean_false{context.value().is_boolean() &&
                                        !context.value().to_boolean()};

  // When `additionalProperties: false` sits with `properties` alone and
  // `properties` compiles to its closed form, that form already rejects
  // unknown properties, so `additionalProperties` need emit nothing. Both
  // evaluation tracking and a defined `patternProperties` hold `properties`
  // back to its non-closed form, in which case the closure must still be
  // emitted here. An empty `patternProperties` contributes no property filters
  // yet still counts as defined, so its mere presence, not the filters it
  // yields, is what matters
  if (context.mode() == Mode::FastValidation && !track_evaluation &&
      rejects_with_boolean_false && children.size() == 1 &&
      children.front().type == InstructionIndex::AssertionFail &&
      !filter_strings.empty() &&
      !context.schema().defines("patternProperties") &&
      properties_as_loop(context, context.schema().at("properties"))) {
    return {};
  }

  // When all properties are required and `additionalProperties: false`, the
  // object is closed by another keyword, so we don't need to emit anything for
  // `additionalProperties`. This happens either because `required` compiles to
  // an `AssertionDefinesExactly` variant (only when there is more than one
  // required property) or because `properties` itself compiles to a closed
  // form. With a single required property `required` only compiles to
  // `AssertionDefinesStrict`, which does not reject unknown properties, so we
  // must still emit the closure unless `properties` enforces it.
  if (context.mode() == Mode::FastValidation && children.size() == 1 &&
      children.front().type == InstructionIndex::AssertionFail &&
      !filter_strings.empty() && filter_prefixes.empty() &&
      filter_regexes.empty()) {
    const auto required{required_properties(context)};
    if (is_closed_properties_required(context.schema(), required) &&
        (required.size() > 1 ||
         (properties_enforce_closed_object(context) &&
          !annotations_enabled(context, "properties")))) {
      return {};
    }
  }

  // Similarly, `patternProperties` only compiles to its closed form, which
  // rejects unmatched properties on its own, when `additionalProperties` is
  // the boolean `false`
  if (context.mode() == Mode::FastValidation && filter_strings.empty() &&
      filter_prefixes.empty() && filter_regexes.size() == 1 &&
      !track_evaluation && rejects_with_boolean_false && !children.empty() &&
      children.front().type == InstructionIndex::AssertionFail) {
    return {};
  }

  if (!filter_strings.empty() || !filter_prefixes.empty() ||
      !filter_regexes.empty()) {
    if (track_evaluation) {
      children.push_back(make(InstructionIndex::ControlEvaluate,
                              context.relative(), ValuePointer{}));
    }

    return {make(InstructionIndex::LoopPropertiesExcept, context,
                 ValuePropertyFilter{std::move(filter_strings),
                                     std::move(filter_prefixes),
                                     std::move(filter_regexes)},
                 std::move(children))};
  }
  if (track_evaluation) {
    // An unconditional marker records this instance as evaluated whatever its
    // type, so when this schema is applied to a non-object, such as a
    // conditional branch evaluated against an array, it would wrongly mark that
    // array evaluated and suppress a sibling `unevaluatedItems`. An
    // object-guarded marker only records objects, and needs no children
    return {make(InstructionIndex::LoopPropertiesEvaluate, context, ValueNone{},
                 std::move(children))};
  }
  if (children.size() == 1 &&
      children.front().type == InstructionIndex::AssertionFail) {
    return {make(InstructionIndex::AssertionObjectSizeLess, context,
                 ValueUnsignedInteger{1})};
  }
  return {make(InstructionIndex::LoopProperties, context, ValueNone{},
               std::move(children))};
}

auto compiler_draft3_applicator_additionalproperties(
    const KeywordContext &context, const Instructions &) -> Instructions {
  return compiler_draft3_applicator_additionalproperties_with_options(
      context, false, false);
}

auto compiler_draft3_validation_pattern(const KeywordContext &context,
                                        const Instructions &) -> Instructions {
  if (!context.value().is_string()) {
    throw_keyword_error(context, EXPECTED_STRING);
  }

  if (narrows_type_away(context.schema(), {"string"})) {
    return {};
  }

  const auto &regex_string{context.value().to_string()};
  return {make(InstructionIndex::AssertionRegex, context,
               ValueRegex{.first = parse_regex(regex_string, context,
                                               context.local.relative_pointer),
                          .second = regex_string})};
}

auto compiler_draft3_applicator_items_array(const KeywordContext &context,
                                            const bool annotate,
                                            const bool track_evaluation)
    -> Instructions {
  if (context.local.is_property_name) {
    return {};
  }

  if (!context.value().is_array()) {
    throw_keyword_error(context, EXPECTED_ARRAY);
  }

  // Draft 3 puts no lower bound on how many schemas the tuple form may list,
  // whereas every dialect after it asks for at least one
  const auto allows_empty{is_draft3(context.vocabularies())};
  const auto &items{context.value()};
  if ((items.empty() && !allows_empty) ||
      !all_are_schemas(items, context.vocabularies())) {
    throw_keyword_error(context, allows_empty
                                     ? EXPECTED_SCHEMA_ARRAY
                                     : EXPECTED_SCHEMA_ARRAY_NON_EMPTY);
  }

  const auto items_size{context.value().size()};

  if (items_size == 0) {
    return {};
  }

  if (narrows_type_away(context.schema(), {"array"})) {
    return {};
  }

  const bool emit_annotation{annotate && annotations_enabled(context)};

  // Precompile subschemas
  std::vector<Instructions> subschemas;
  subschemas.reserve(items_size);
  const auto &array{context.value().as_array()};
  for (auto iterator{array.cbegin()}; iterator != array.cend(); ++iterator) {
    subschemas.push_back(
        compile(context.relative(), {subschemas.size()}, {subschemas.size()}));
  }

  Instructions children;
  for (std::size_t cursor = 0; cursor < items_size; cursor++) {
    Instructions subchildren;
    for (std::size_t index = 0; index < cursor + 1; index++) {
      for (const auto &substep : subschemas.at(index)) {
        subchildren.push_back(substep);
      }
    }

    if (emit_annotation) {
      subchildren.push_back(make(InstructionIndex::AnnotationEmit,
                                 context.relative(),
                                 sourcemeta::core::JSON{cursor}));
    }

    children.push_back(make(InstructionIndex::ControlGroup, context.relative(),
                            ValueNone{}, std::move(subchildren)));
  }

  Instructions tail;
  for (const auto &subschema : subschemas) {
    for (const auto &substep : subschema) {
      tail.push_back(substep);
    }
  }

  if (emit_annotation) {
    tail.push_back(make(InstructionIndex::AnnotationEmit, context.relative(),
                        sourcemeta::core::JSON{children.size() - 1}));
    tail.push_back(make(InstructionIndex::AnnotationEmit, context.relative(),
                        sourcemeta::core::JSON{true}));
  }

  children.push_back(make(InstructionIndex::ControlGroup, context.relative(),
                          ValueNone{}, std::move(tail)));

  if (track_evaluation) {
    return {make(InstructionIndex::AssertionArrayPrefixEvaluate, context,
                 ValueNone{}, std::move(children))};
  }
  return {make(InstructionIndex::AssertionArrayPrefix, context, ValueNone{},
               std::move(children))};
}

auto is_number_type_check(const Instruction &instruction) -> bool {
  if (instruction.type != InstructionIndex::AssertionTypeStrictAny) {
    return false;
  }

  const auto &value{std::get<ValueTypes>(instruction.value)};
  const auto numeric_count{
      static_cast<std::size_t>(value.test(
          std::to_underlying(sourcemeta::core::JSON::Type::Integer))) +
      static_cast<std::size_t>(
          value.test(std::to_underlying(sourcemeta::core::JSON::Type::Real))) +
      static_cast<std::size_t>(value.test(
          std::to_underlying(sourcemeta::core::JSON::Type::Decimal)))};
  return numeric_count >= 2 && value.count() == numeric_count;
}

auto is_integer_bounded_pattern(const Instructions &children) -> bool {
  if (children.size() != 3) {
    return false;
  }

  bool has_type{false};
  bool has_min{false};
  bool has_max{false};
  for (const auto &child : children) {
    if (is_number_type_check(child)) {
      has_type = true;
    } else if (child.type == InstructionIndex::AssertionGreaterEqual) {
      if (!std::get<ValueJSON>(child.value).is_integer()) {
        return false;
      }
      has_min = true;
    } else if (child.type == InstructionIndex::AssertionLessEqual) {
      if (!std::get<ValueJSON>(child.value).is_integer()) {
        return false;
      }
      has_max = true;
    }
  }

  return has_type && has_min && has_max;
}

auto is_bounded_array_items_pattern(const Instructions &children) -> bool {
  if (children.size() != 2) {
    return false;
  }

  bool has_types{false};
  bool has_bounds{false};
  for (const auto &child : children) {
    // A child that looks anywhere other than the item itself is not part of
    // the shape, whatever its type says
    if (!child.relative_instance_location.empty()) {
      return false;
    }

    if (child.type == InstructionIndex::LoopItemsTypeStrictAny) {
      has_types = true;
    } else if (child.type == InstructionIndex::AssertionTypeArrayBounded) {
      // The fused instruction stops at the first item that does not match,
      // which an exhaustive bound does not allow
      if (std::get<2>(std::get<ValueRange>(child.value))) {
        return false;
      }

      has_bounds = true;
    }
  }

  return has_types && has_bounds;
}

auto extract_bounded_array_items(const Instructions &children)
    -> ValueTypesWithSize {
  ValueTypes types;
  ValueRange bounds;
  for (const auto &child : children) {
    if (child.type == InstructionIndex::LoopItemsTypeStrictAny) {
      types = std::get<ValueTypes>(child.value);
    } else if (child.type == InstructionIndex::AssertionTypeArrayBounded) {
      bounds = std::get<ValueRange>(child.value);
    }
  }

  return {types, bounds};
}

auto compiler_draft3_applicator_items_with_options(
    const KeywordContext &context, const bool annotate,
    const bool track_evaluation) -> Instructions {
  // The tuple form must be a well-formed list of schemas whatever the instance
  // turns out to be, so this runs before the check on which instances the
  // keyword can apply to. Draft 3 puts no lower bound on how many it may list,
  // whereas every dialect after it asks for at least one
  if (context.value().is_array()) {
    const auto allows_empty{is_draft3(context.vocabularies())};
    const auto &entries{context.value()};
    if ((entries.empty() && !allows_empty) ||
        !all_are_schemas(entries, context.vocabularies())) {
      throw_keyword_error(context, allows_empty
                                       ? EXPECTED_SCHEMA_ARRAY
                                       : EXPECTED_SCHEMA_ARRAY_NON_EMPTY);
    }
  }

  if (narrows_type_away(context.schema(), {"array"})) {
    compile_pruned_subschema(context, sourcemeta::core::EMPTY_WEAK_POINTER);
    return {};
  }

  const bool emit_annotation{annotate && annotations_enabled(context)};

  if (context.value().is_object() || context.value().is_boolean()) {
    if (emit_annotation || track_evaluation) {
      Instructions subchildren{compile(context.relative(),
                                       sourcemeta::core::EMPTY_WEAK_POINTER,
                                       sourcemeta::core::EMPTY_WEAK_POINTER)};

      Instructions children;

      if (!subchildren.empty()) {
        children.push_back(make(InstructionIndex::LoopItems, context,
                                ValueNone{}, std::move(subchildren)));
      }

      if (!emit_annotation && !track_evaluation) {
        return children;
      }

      Instructions tail;

      if (emit_annotation) {
        tail.push_back(make(InstructionIndex::AnnotationEmit,
                            context.relative(), sourcemeta::core::JSON{true}));
      }

      if (track_evaluation) {
        tail.push_back(make(InstructionIndex::ControlEvaluate,
                            context.relative(), ValuePointer{}));
      }

      children.push_back(make(InstructionIndex::LogicalWhenType, context,
                              sourcemeta::core::JSON::Type::Array,
                              std::move(tail)));

      return children;
    }

    Instructions children{compile(context.relative(),
                                  sourcemeta::core::EMPTY_WEAK_POINTER,
                                  sourcemeta::core::EMPTY_WEAK_POINTER)};
    if (track_evaluation) {
      children.push_back(make(InstructionIndex::ControlEvaluate,
                              context.relative(), ValuePointer{}));
    }

    if (children.empty()) {
      return {};
    }

    if (context.mode() == Mode::FastValidation && children.size() == 3 &&
        is_integer_bounded_pattern(children)) {
      return {make(InstructionIndex::LoopItemsIntegerBounded, context,
                   extract_integer_bounds(children))};
    }

    if (context.mode() == Mode::FastValidation && children.size() == 2 &&
        is_bounded_array_items_pattern(children)) {
      return {make(InstructionIndex::LoopItemsArrayBoundedTypeStrictAny,
                   context, extract_bounded_array_items(children))};
    }

    if (context.mode() == Mode::FastValidation && children.size() == 1) {
      if (children.front().type == InstructionIndex::AssertionTypeStrict) {
        return {make(InstructionIndex::LoopItemsTypeStrict, context,
                     children.front().value)};
      }
      if (children.front().type == InstructionIndex::AssertionType) {
        return {make(InstructionIndex::LoopItemsType, context,
                     children.front().value)};
      }
      if (children.front().type == InstructionIndex::AssertionTypeStrictAny) {
        return {make(InstructionIndex::LoopItemsTypeStrictAny, context,
                     children.front().value)};
      }
      if (children.front().type ==
          InstructionIndex::LoopPropertiesExactlyTypeStrictHash) {
        auto value_copy = children.front().value;
        auto current{make(InstructionIndex::LoopItems, context, ValueNone{},
                          std::move(children))};
        if (std::get<ValueTypedHashes>(value_copy).second.first.size() == 3) {
          return {Instruction{
              .type =
                  InstructionIndex::LoopItemsPropertiesExactlyTypeStrictHash3,
              .relative_instance_location = current.relative_instance_location,
              .value = std::move(value_copy),
              .children = {},
              .extra_index = current.extra_index}};
        }

        return {Instruction{
            .type = InstructionIndex::LoopItemsPropertiesExactlyTypeStrictHash,
            .relative_instance_location = current.relative_instance_location,
            .value = std::move(value_copy),
            .children = {},
            .extra_index = current.extra_index}};
      }
    }

    return {make(InstructionIndex::LoopItems, context, ValueNone{},
                 std::move(children))};
  }

  return compiler_draft3_applicator_items_array(context, annotate,
                                                track_evaluation);
}

auto compiler_draft3_applicator_items(const KeywordContext &context,
                                      const Instructions &) -> Instructions {
  return compiler_draft3_applicator_items_with_options(context, false, false);
}

auto compiler_draft3_applicator_additionalitems_from_cursor(
    const KeywordContext &context, const std::size_t cursor,
    const bool annotate, const bool track_evaluation) -> Instructions {
  if (narrows_type_away(context.schema(), {"array"})) {
    return {};
  }

  const bool emit_annotation{annotate && annotations_enabled(context)};

  Instructions subchildren{compile(context.relative(),
                                   sourcemeta::core::EMPTY_WEAK_POINTER,
                                   sourcemeta::core::EMPTY_WEAK_POINTER)};

  Instructions children;

  if (!subchildren.empty()) {
    if (context.mode() == Mode::FastValidation && cursor == 0 &&
        !emit_annotation && !track_evaluation &&
        is_integer_bounded_pattern(subchildren)) {
      children.push_back(make(InstructionIndex::LoopItemsIntegerBounded,
                              context, extract_integer_bounds(subchildren)));
      return children;
    }

    children.push_back(make(InstructionIndex::LoopItemsFrom, context,
                            ValueUnsignedInteger{cursor},
                            std::move(subchildren)));
  }

  // Avoid one extra wrapper instruction if possible
  if (!emit_annotation && !track_evaluation) {
    return children;
  }

  Instructions tail;

  if (emit_annotation) {
    tail.push_back(make(InstructionIndex::AnnotationEmit, context.relative(),
                        sourcemeta::core::JSON{true}));
  }

  if (track_evaluation) {
    tail.push_back(make(InstructionIndex::ControlEvaluate, context.relative(),
                        ValuePointer{}));
  }

  assert(!tail.empty());
  children.push_back(make(InstructionIndex::LogicalWhenArraySizeGreater,
                          context, ValueUnsignedInteger{cursor},
                          std::move(tail)));

  return children;
}

auto compiler_draft3_applicator_additionalitems_with_options(
    const KeywordContext &context, const bool annotate,
    const bool track_evaluation) -> Instructions {
  if (narrows_type_away(context.schema(), {"array"})) {
    return {};
  }

  assert(context.schema().is_object());

  // This keyword only has a say alongside an `items` array, as every other
  // form of `items` already applies to every element. Draft 6 made a boolean a
  // schema in its own right, so `items: true` is one of those forms rather
  // than an absent `items`
  if (!context.schema().defines("items") ||
      !context.schema().at("items").is_array()) {
    return {};
  }

  const auto cursor{context.schema().at("items").size()};

  return compiler_draft3_applicator_additionalitems_from_cursor(
      context, cursor, annotate, track_evaluation);
}

auto compiler_draft3_applicator_additionalitems(const KeywordContext &context,
                                                const Instructions &)
    -> Instructions {
  return compiler_draft3_applicator_additionalitems_with_options(context, false,
                                                                 false);
}

auto compiler_draft3_validation_enum(const KeywordContext &context,
                                     const Instructions &) -> Instructions {
  if (!context.value().is_array()) {
    throw_keyword_error(context, EXPECTED_ARRAY);
  }

  if ((is_draft3(context.vocabularies()) || is_draft4(context.vocabularies()) ||
       is_draft6(context.vocabularies()) ||
       is_draft7(context.vocabularies())) &&
      (context.value().empty() || !context.value().unique())) {
    throw_keyword_error(context, EXPECTED_UNIQUE_VALUES);
  }

  // The OpenAPI 3.0 Schema Object asks for at least one value, and is alone in
  // permitting duplicates among them
  if (is_openapi_3_0(context.vocabularies()) && context.value().empty()) {
    throw_keyword_error(context, EXPECTED_NON_EMPTY_VALUES);
  }

  if (context.value().empty()) {
    return {make(InstructionIndex::AssertionFail, context, ValueNone{})};
  }

  if (context.value().size() == 1) {
    return {make(InstructionIndex::AssertionEqual, context,
                 sourcemeta::core::JSON{context.value().front()})};
  }

  std::vector<std::pair<ValueString, ValueStringSet::hash_type>>
      perfect_string_hashes;
  ValueSet options;
  sourcemeta::core::PropertyHashJSON<ValueString> hasher;
  for (const auto &option : context.value().as_array()) {
    // Neither 2019-09 onwards nor the OpenAPI 3.0 Schema Object asks for the
    // options to be unique, and the comparison below weighs these hashes
    // against the deduplicated set of options, so a repeated option must not
    // be counted twice
    if (!options.insert(option).second) {
      continue;
    }

    if (option.is_string()) {
      const auto hash{hasher(option.to_string())};
      if (hasher.is_perfect(hash)) {
        perfect_string_hashes.emplace_back(option.to_string(), hash);
      }
    }
  }

  // Only apply this optimisation on fast validation, as it
  // can affect error messages
  if (context.mode() == Mode::FastValidation &&
      perfect_string_hashes.size() == options.size()) {
    return {make(InstructionIndex::AssertionEqualsAnyStringHash, context,
                 to_string_hashes(perfect_string_hashes))};
  }

  return {
      make(InstructionIndex::AssertionEqualsAny, context, std::move(options))};
}

auto compiler_draft3_validation_uniqueitems(const KeywordContext &context,
                                            const Instructions &)
    -> Instructions {
  if (!context.value().is_boolean() || !context.value().to_boolean()) {
    return {};
  }

  if (narrows_type_away(context.schema(), {"array"})) {
    return {};
  }

  return {make(InstructionIndex::AssertionUnique, context, ValueNone{})};
}

// `maxLength`, `maxItems` and `maxProperties`, along with their lower-bound
// counterparts, differ only in the type whose size they bound. Only a bound on
// the size of a string says anything about a property name, which is always a
// string, so only that one has to stand its ground where the type assertion
// would otherwise absorb it
struct SizeBound {
  sourcemeta::core::JSON::Type type;
  std::string_view name;
  InstructionIndex instruction;
};

// Draft 3 asks only that `maxLength` be an integer, unlike the bounds around
// it, which it also asks to be non-negative. Pass `tolerate_negative` there,
// as a negative upper bound is one that no value of the type can ever be
// small enough to meet
auto compile_size_upper_bound(const KeywordContext &context,
                              const SizeBound &bound,
                              const bool tolerate_negative = false)
    -> Instructions {
  const auto &value{context.value()};
  if (!is_dialect_integer(value, context.vocabularies())) {
    throw_keyword_error(context, EXPECTED_INTEGER);
  }

  if (!tolerate_negative && !value.is_positive()) {
    throw_keyword_error(context, EXPECTED_NON_NEGATIVE);
  }

  if (narrows_type_away(context.schema(), {bound.name})) {
    return {};
  }

  // Converting a negative bound would wrap around and drop the constraint, so
  // we say what it means instead: every value of the type fails, and nothing
  // else is affected
  if (!value.is_positive()) {
    ValueTypes types;
    types.set(std::to_underlying(bound.type));
    return {make(InstructionIndex::AssertionNotTypeStrictAny, context, types)};
  }

  // We'll handle it at the type level as an optimization. Note that the type
  // compiler bails out before it reads these bounds when it is compiling a
  // property name, so there a string bound must still emit itself
  if (context.mode() == Mode::FastValidation &&
      !(context.local.is_property_name &&
        bound.type == sourcemeta::core::JSON::Type::String) &&
      assume_type(context.vocabularies(), context.schema(), bound.name)) {
    return {};
  }

  // A bound too large to hold sits beyond the size of anything that can
  // exist, so every value of the type meets it and there is nothing left to
  // assert
  if (!bound_is_representable(value)) {
    return {};
  }

  return {make(bound.instruction, context,
               ValueUnsignedInteger{
                   static_cast<unsigned long>(value.as_integer()) + 1})};
}

auto compile_size_lower_bound(const KeywordContext &context,
                              const SizeBound &bound) -> Instructions {
  const auto &value{context.value()};
  if (!is_dialect_integer(value, context.vocabularies())) {
    throw_keyword_error(context, EXPECTED_INTEGER);
  }

  if (!value.is_positive()) {
    throw_keyword_error(context, EXPECTED_NON_NEGATIVE);
  }

  if (narrows_type_away(context.schema(), {bound.name})) {
    return {};
  }

  // A bound too large to hold is beyond the size of anything that can exist,
  // so no value of the type can meet it, and nothing else is affected. A
  // property name is always a string, so a string bound leaves it no way out
  // while any other bound has nothing to say about it
  if (!bound_is_representable(value)) {
    if (context.local.is_property_name) {
      if (bound.type != sourcemeta::core::JSON::Type::String) {
        return {};
      }

      return {make(InstructionIndex::AssertionFail, context, ValueNone{})};
    }

    ValueTypes types;
    types.set(std::to_underlying(bound.type));
    return {make(InstructionIndex::AssertionNotTypeStrictAny, context, types)};
  }

  // We'll handle it at the type level as an optimization. Note that the type
  // compiler bails out before it reads these bounds when it is compiling a
  // property name, so there a string bound must still emit itself
  if (context.mode() == Mode::FastValidation &&
      !(context.local.is_property_name &&
        bound.type == sourcemeta::core::JSON::Type::String) &&
      assume_type(context.vocabularies(), context.schema(), bound.name)) {
    return {};
  }

  const auto bound_value{value.as_integer()};
  if (bound_value <= 0) {
    return {};
  }

  return {
      make(bound.instruction, context,
           ValueUnsignedInteger{static_cast<unsigned long>(bound_value - 1)})};
}

auto compiler_draft3_validation_maxlength(const KeywordContext &context,
                                          const Instructions &)
    -> Instructions {
  return compile_size_upper_bound(
      context,
      {.type = sourcemeta::core::JSON::Type::String,
       .name = "string",
       .instruction = InstructionIndex::AssertionStringSizeLess},
      is_draft3(context.vocabularies()));
}

auto compiler_draft3_validation_minlength(const KeywordContext &context,
                                          const Instructions &)
    -> Instructions {
  return compile_size_lower_bound(
      context, {.type = sourcemeta::core::JSON::Type::String,
                .name = "string",
                .instruction = InstructionIndex::AssertionStringSizeGreater});
}

auto compiler_draft3_validation_maxitems(const KeywordContext &context,
                                         const Instructions &) -> Instructions {
  return compile_size_upper_bound(
      context, {.type = sourcemeta::core::JSON::Type::Array,
                .name = "array",
                .instruction = InstructionIndex::AssertionArraySizeLess});
}

auto compiler_draft3_validation_minitems(const KeywordContext &context,
                                         const Instructions &) -> Instructions {
  return compile_size_lower_bound(
      context, {.type = sourcemeta::core::JSON::Type::Array,
                .name = "array",
                .instruction = InstructionIndex::AssertionArraySizeGreater});
}

auto compiler_draft3_validation_maximum(const KeywordContext &context,
                                        const Instructions &) -> Instructions {
  if (!context.value().is_number()) {
    throw_keyword_error(context, EXPECTED_NUMBER);
  }

  if (narrows_type_away(context.schema(), {"integer", "number"})) {
    return {};
  }

  // TODO: As an optimization, if `minimum` is set to the same number, do
  // a single equality assertion

  assert(context.schema().is_object());
  if (context.schema().defines("exclusiveMaximum") &&
      context.schema().at("exclusiveMaximum").is_boolean() &&
      context.schema().at("exclusiveMaximum").to_boolean()) {
    return {make(InstructionIndex::AssertionLess, context,
                 sourcemeta::core::JSON{context.value()})};
  }
  return {make(InstructionIndex::AssertionLessEqual, context,
               sourcemeta::core::JSON{context.value()})};
}

auto compiler_draft3_validation_minimum(const KeywordContext &context,
                                        const Instructions &) -> Instructions {
  if (!context.value().is_number()) {
    throw_keyword_error(context, EXPECTED_NUMBER);
  }

  if (narrows_type_away(context.schema(), {"integer", "number"})) {
    return {};
  }

  // TODO: As an optimization, if `maximum` is set to the same number, do
  // a single equality assertion

  assert(context.schema().is_object());
  if (context.schema().defines("exclusiveMinimum") &&
      context.schema().at("exclusiveMinimum").is_boolean() &&
      context.schema().at("exclusiveMinimum").to_boolean()) {
    return {make(InstructionIndex::AssertionGreater, context,
                 sourcemeta::core::JSON{context.value()})};
  }
  return {make(InstructionIndex::AssertionGreaterEqual, context,
               sourcemeta::core::JSON{context.value()})};
}

auto compiler_draft3_validation_type(const KeywordContext &context,
                                     const Instructions &) -> Instructions {
  const auto &value{context.value()};

  const auto draft3{is_draft3(context.vocabularies())};

  // This dialect types `type` as a string drawn from six names. It has no
  // array form and no null type, offering `nullable` in place of the latter.
  // `draft3_set_type_bits` answers for the seven JSON Schema names, so the
  // null one has to be ruled out separately
  if (is_openapi_3_0(context.vocabularies())) {
    ValueTypes recognised{};
    if (!value.is_string() || value.to_string() == "null" ||
        !draft3_set_type_bits(value.to_string(), recognised)) {
      throw_keyword_error(context, UNSUPPORTED_VALUE_FOR_DIALECT);
    }

    // A scalar `type` widened by `nullable` is the array form in all but
    // spelling, so it takes the same instruction and skips the scalar fast
    // paths below, none of which can express a bound alongside a second
    // permitted type
    if (is_nullable(context.vocabularies(), context.schema())) {
      ValueTypes types{recognised};
      types.set(std::to_underlying(sourcemeta::core::JSON::Type::Null));
      return {make(InstructionIndex::AssertionTypeStrictAny, context, types)};
    }
  }

  if (draft3) {
    if (value.is_string() && value.to_string() == "any") {
      return draft3_any_type_instructions(context);
    }

    if (value.is_array()) {
      if (!is_draft3_type_union(value)) {
        throw_keyword_error(context, EXPECTED_TYPE_NAMES);
      }

      bool has_object{false};
      for (const auto &element : value.as_array()) {
        if (element.is_string() && element.to_string() == "any") {
          return draft3_any_type_instructions(context);
        }
        if (element.is_object()) {
          has_object = true;
        }
      }

      if (has_object) {
        if (context.mode() == Mode::FastValidation && value.size() == 1) {
          return compile(
              context,
              {static_cast<sourcemeta::core::Pointer::Token::Index>(0)});
        }

        Instructions disjunctors;
        for (std::uint64_t index = 0; index < value.size(); index++) {
          const auto &element{value.at(index)};
          Instructions branch;

          if (element.is_object()) {
            branch = compile(
                context.relative(),
                {static_cast<sourcemeta::core::Pointer::Token::Index>(index)});
          } else if (element.is_string()) {
            auto assertion{
                make_type_assertion(context.relative(), element.to_string())};
            if (!assertion.has_value()) {
              continue;
            }

            branch.push_back(std::move(assertion).value());
          } else {
            continue;
          }

          disjunctors.push_back(make(InstructionIndex::ControlGroup,
                                     context.relative(), ValueNone{},
                                     std::move(branch)));
        }

        return {make(InstructionIndex::LogicalOr, context, ValueBoolean{false},
                     std::move(disjunctors))};
      }
    }
  }

  if (value.is_string()) {
    const auto &type{value.to_string()};
    if (type == "null") {
      if (type_covered_by_enum(context, "null")) {
        return {};
      }

      return {make(InstructionIndex::AssertionTypeStrict, context,
                   sourcemeta::core::JSON::Type::Null)};
    }
    if (type == "boolean") {
      if (type_covered_by_enum(context, "boolean")) {
        return {};
      }

      return {make(InstructionIndex::AssertionTypeStrict, context,
                   sourcemeta::core::JSON::Type::Boolean)};
    }
    if (type == "object") {
      if (!draft3) {
        const auto minimum{
            unsigned_integer_property(context.schema(), "minProperties", 0)};
        const auto maximum{
            unsigned_integer_property(context.schema(), "maxProperties")};

        const auto fused{
            fuse_type_size_bound(context, minimum, maximum,
                                 InstructionIndex::AssertionTypeObjectUpper,
                                 InstructionIndex::AssertionTypeObjectBounded)};
        if (fused.has_value()) {
          return fused.value();
        }
      }

      if (type_covered_by_enum(context, "object")) {
        return {};
      }

      // A non-empty `required` rejects a non-object on its own, so the type
      // assertion is redundant. An empty `required` asserts nothing, so the
      // type check must stay
      if (!draft3 && context.mode() == Mode::FastValidation &&
          context.schema().defines("required") &&
          context.schema().at("required").is_array() &&
          !context.schema().at("required").empty()) {
        return {};
      }

      return {make(InstructionIndex::AssertionTypeStrict, context,
                   sourcemeta::core::JSON::Type::Object)};
    }
    if (type == "array") {
      const auto minimum{
          unsigned_integer_property(context.schema(), "minItems", 0)};
      const auto maximum{
          unsigned_integer_property(context.schema(), "maxItems")};

      const auto fused{fuse_type_size_bound(
          context, minimum, maximum, InstructionIndex::AssertionTypeArrayUpper,
          InstructionIndex::AssertionTypeArrayBounded)};
      if (fused.has_value()) {
        return fused.value();
      }

      if (type_covered_by_enum(context, "array")) {
        return {};
      }

      return {make(InstructionIndex::AssertionTypeStrict, context,
                   sourcemeta::core::JSON::Type::Array)};
    }
    if (type == "number") {
      if (type_covered_by_enum(context, "number")) {
        return {};
      }

      ValueTypes types{};
      types.set(std::to_underlying(sourcemeta::core::JSON::Type::Real));
      types.set(std::to_underlying(sourcemeta::core::JSON::Type::Integer));
      types.set(std::to_underlying(sourcemeta::core::JSON::Type::Decimal));
      return {make(InstructionIndex::AssertionTypeStrictAny, context, types)};
    }
    if (type == "integer") {
      // Draft 4 and earlier have no shortcut for an all-integer `enum` here,
      // and neither does the OpenAPI 3.0 Schema Object, unlike every other
      // type name. Equality reads a number by its value rather than by how it
      // is written, so the options admit an instance carrying a fractional
      // part, which none of these dialects counts as an integer, and so this
      // assertion always has work left to do
      return {make(InstructionIndex::AssertionTypeStrict, context,
                   sourcemeta::core::JSON::Type::Integer)};
    }
    if (type == "string") {
      const auto minimum{
          unsigned_integer_property(context.schema(), "minLength", 0)};
      const auto maximum{
          unsigned_integer_property(context.schema(), "maxLength")};

      const auto fused{fuse_type_size_bound(
          context, minimum, maximum, InstructionIndex::AssertionTypeStringUpper,
          InstructionIndex::AssertionTypeStringBounded)};
      if (fused.has_value()) {
        return fused.value();
      }

      if (type_covered_by_enum(context, "string")) {
        return {};
      }

      return {make(InstructionIndex::AssertionTypeStrict, context,
                   sourcemeta::core::JSON::Type::String)};
    }
    if (draft3) {
      // Draft 3 puts no enumeration on `type`, so any name at all satisfies
      // the meta-schema, and one we do not know constrains nothing
      return {};
    }
    throw_keyword_error(context, EXPECTED_TYPE_NAMES);
  }
  if (value.is_array() && value.size() == 1 && value.front().is_string()) {
    auto assertion{make_type_assertion(context, value.front().to_string())};
    if (assertion.has_value()) {
      return {std::move(assertion).value()};
    }

    if (draft3) {
      // Draft 3 puts no enumeration on `type`, so any name at all satisfies
      // the meta-schema, and one we do not know constrains nothing
      return {};
    }

    throw_keyword_error(context, EXPECTED_TYPE_NAMES);
  }
  if (value.is_array()) {
    // Draft 4 asks for a non-empty array of unique type names, whereas Draft 3
    // also admits a subschema as an alternative and tolerates an empty union
    if (!value.unique() || (!draft3 && value.empty()) ||
        !std::ranges::all_of(
            value.as_array(), [draft3](const auto &element) -> bool {
              return element.is_string() || (draft3 && element.is_object());
            })) {
      throw_keyword_error(context, EXPECTED_TYPE_NAMES);
    }

    ValueTypes types{};
    for (const auto &element : value.as_array()) {
      if (!element.is_string()) {
        continue;
      }

      if (!draft3_set_type_bits(element.to_string(), types) && !draft3) {
        throw_keyword_error(context, EXPECTED_TYPE_NAMES);
      }
    }

    if (!types.any()) {
      // An empty array names no type at all, so no value can match it. A
      // non-empty one that named only unrecognised types is an invalid but
      // legitimate use of the keyword that we ignore, constraining nothing,
      // exactly as the scalar form does for a single unrecognised name
      if (value.empty()) {
        return {make(InstructionIndex::AssertionFail, context, ValueNone{})};
      }

      return {};
    }

    return {make(InstructionIndex::AssertionTypeStrictAny, context, types)};
  }

  // Every dialect asks that `type` be a type name or an array of them
  throw_keyword_error(context, EXPECTED_TYPE_NAMES);
}

auto compiler_draft3_validation_disallow(const KeywordContext &context,
                                         const Instructions &) -> Instructions {
  const auto &value{context.value()};

  // Draft 3 gives `disallow` the same shape it gives `type`: a type name, or
  // an array of names and subschemas
  if (!value.is_string() && !is_draft3_type_union(value)) {
    throw_keyword_error(context, EXPECTED_TYPE_NAMES);
  }

  const auto contains_any{
      (value.is_string() && value.to_string() == "any") ||
      (value.is_array() &&
       std::ranges::any_of(value.as_array(), [](const auto &element) -> auto {
         return element.is_string() && element.to_string() == "any";
       }))};
  if (contains_any) {
    return {make(InstructionIndex::AssertionFail, context, ValueNone{})};
  }

  ValueTypes types{};
  Instructions subschema_nots;

  if (value.is_string()) {
    draft3_set_type_bits(value.to_string(), types);
  } else if (value.is_array()) {
    for (std::uint64_t index = 0; index < value.size(); index++) {
      const auto &element{value.at(index)};
      if (element.is_string()) {
        draft3_set_type_bits(element.to_string(), types);
      } else if (element.is_object()) {
        sourcemeta::core::WeakPointer index_suffix;
        index_suffix.push_back(index);
        const auto element_uri{
            sourcemeta::core::to_uri(
                context.local.relative_pointer.concat(index_suffix),
                context.base())
                .canonicalize()
                .recompose()};

        auto inner_instructions{
            compile(context.relative(), sourcemeta::core::EMPTY_WEAK_POINTER,
                    sourcemeta::core::EMPTY_WEAK_POINTER, element_uri)};

        const auto element_relative_pointer{
            context.local.relative_pointer.concat(index_suffix)};
        const SchemaContext element_schema_context{
            .relative_pointer = element_relative_pointer,
            .schema = context.schema(),
            .vocabularies = context.vocabularies(),
            .base = context.base(),
            .is_property_name = context.local.is_property_name};

        const auto element_base_schema_location{
            make_weak_pointer(context.keyword(), index)};
        const KeywordContext element_context{
            .global = context.global,
            .local = element_schema_context,
            .target = {.keyword = EMPTY_KEYWORD,
                       .base_schema_location = element_base_schema_location,
                       .base_instance_location =
                           context.target.base_instance_location}};

        subschema_nots.push_back(make(InstructionIndex::LogicalNot,
                                      element_context, ValueNone{},
                                      std::move(inner_instructions)));
      }
    }
  }

  Instructions result;
  if (types.any()) {
    result.push_back(
        make(InstructionIndex::AssertionNotTypeStrictAny, context, types));
  }
  for (auto &&instruction : subschema_nots) {
    result.push_back(std::move(instruction));
  }
  return result;
}

auto compiler_draft3_applicator_extends(const KeywordContext &context,
                                        const Instructions &) -> Instructions {
  const auto &value{context.value()};

  // Unlike the keywords that share this shape, Draft 3 puts no lower bound on
  // how many schemas `extends` may combine
  if (value.is_array() && !all_are_schemas(value, context.vocabularies())) {
    throw_keyword_error(context, EXPECTED_SCHEMA_ARRAY);
  }

  if (value.is_object()) {
    // TODO: Make this work with `$dynamicRef`
    if (context.mode() == Mode::FastValidation &&
        !context.global.uses_dynamic_scopes) {
      return compile(context, sourcemeta::core::EMPTY_WEAK_POINTER,
                     sourcemeta::core::EMPTY_WEAK_POINTER);
    }

    auto inner{compile(context.relative(), sourcemeta::core::EMPTY_WEAK_POINTER,
                       sourcemeta::core::EMPTY_WEAK_POINTER)};
    if (inner.empty()) {
      return {};
    }

    return {make(InstructionIndex::LogicalAnd, context, ValueNone{},
                 std::move(inner))};
  }

  if (!value.is_array()) {
    return {};
  }

  if (value.empty()) {
    return {};
  }

  Instructions children;

  // TODO: Make this work with `$dynamicRef`
  if (context.mode() == Mode::FastValidation &&
      !context.global.uses_dynamic_scopes) {
    for (std::uint64_t index = 0; index < value.size(); index++) {
      for (auto &&step : compile(
               context,
               {static_cast<sourcemeta::core::Pointer::Token::Index>(index)})) {
        children.push_back(std::move(step));
      }
    }

    return children;
  }

  for (std::uint64_t index = 0; index < value.size(); index++) {
    auto arm{
        compile(context.relative(),
                {static_cast<sourcemeta::core::Pointer::Token::Index>(index)})};
    if (arm.empty()) {
      continue;
    }
    children.push_back(make(InstructionIndex::ControlGroup, context.relative(),
                            ValueNone{}, std::move(arm)));
  }

  if (children.empty()) {
    return {};
  }

  return {make(InstructionIndex::LogicalAnd, context, ValueNone{},
               std::move(children))};
}

auto compiler_draft3_applicator_dependencies(const KeywordContext &context,
                                             const Instructions &)
    -> Instructions {
  if (!context.value().is_object()) {
    throw_keyword_error(context, EXPECTED_OBJECT);
  }

  const auto draft3{is_draft3(context.vocabularies())};

  // Every member must be a schema or a list of property names, plus a single
  // property name in Draft 3. Draft 4 onwards ask that such a list have no
  // repeats, which Draft 3 does not ask for at all. A member that is none of
  // those makes the keyword as a whole stop being a constraint
  if (!std::ranges::all_of(context.value().as_object(),
                           [draft3, &vocabularies = context.vocabularies()](
                               const auto &entry) -> bool {
                             return sourcemeta::core::schema_is_schema(
                                        entry.second, vocabularies) ||
                                    (is_property_name_array(entry.second) &&
                                     (draft3 || entry.second.unique())) ||
                                    (draft3 && entry.second.is_string());
                           })) {
    throw_keyword_error(context, draft3 ? EXPECTED_DEPENDENCIES_DRAFT3
                                        : EXPECTED_DEPENDENCIES);
  }

  // Only once the shape is known to be good does it matter whether the keyword
  // can apply to the instances this schema admits
  if (narrows_type_away(context.schema(), {"object"})) {
    return {};
  }

  Instructions children;
  ValueStringMap dependencies;

  for (const auto &entry : context.value().as_object()) {
    if ((entry.second.is_object() || entry.second.is_boolean())) {
      if (!entry.second.is_boolean() || !entry.second.to_boolean()) {
        children.push_back(
            make(InstructionIndex::LogicalWhenDefines, context,
                 make_property(entry.first),
                 compile(context.relative(), make_weak_pointer(entry.first))));
      }
    } else if (entry.second.is_array()) {
      ValueStringSet properties;
      for (const auto &property : entry.second.as_array()) {
        if (!property.is_string()) {
          continue;
        }

        properties.insert(property.to_string());
      }

      if (!properties.empty()) {
        dependencies.emplace(entry.first, std::move(properties));
      }
    } else if (draft3 && entry.second.is_string()) {
      ValueStringSet properties;
      properties.insert(entry.second.to_string());
      dependencies.emplace(entry.first, std::move(properties));
    }
  }

  if (!dependencies.empty()) {
    children.push_back(make(InstructionIndex::AssertionPropertyDependencies,
                            context, std::move(dependencies)));
  }

  return children;
}

auto compiler_draft3_validation_divisibleby(const KeywordContext &context,
                                            const Instructions &)
    -> Instructions {
  if (!context.value().is_number()) {
    throw_keyword_error(context, EXPECTED_NUMBER);
  }

  // The meta-schema asks for a divisor strictly greater than zero, as dividing
  // by zero names no multiple at all
  if (!context.value().is_positive() ||
      context.value() == sourcemeta::core::JSON{0}) {
    throw_keyword_error(context, EXPECTED_POSITIVE);
  }

  if (narrows_type_away(context.schema(), {"integer", "number"})) {
    return {};
  }

  return {make(InstructionIndex::AssertionDivisible, context,
               sourcemeta::core::JSON{context.value()})};
}

// The formats that the OpenAPI 3.1 and 3.2 specifications define for numbers,
// which the rest of this compiler treats apart from the string ones
auto is_openapi_numeric_format(const KeywordContext &context,
                               const sourcemeta::core::JSON &format) -> bool {
  using Known = sourcemeta::core::SchemaVocabularies::Known;
  if (!format.is_string() ||
      (!context.vocabularies().contains(Known::OPENAPI_3_1_BASE) &&
       !context.vocabularies().contains(Known::OPENAPI_3_2_BASE))) {
    return false;
  }

  const auto &name{format.to_string()};
  return name == "int32" || name == "int64" || name == "float" ||
         name == "double";
}

// Every OpenAPI version types `int32`, `int64`, `float`, and `double` over
// numbers, so the assertion each name compiles to is the same wherever it
// appears. The caller answers for `name` being one of the four
auto make_openapi_numeric_format_assertion(
    const KeywordContext &context, const sourcemeta::core::JSON::String &name)
    -> Instruction {
  if (name == "int32") {
    return make(InstructionIndex::AssertionNumberIntegerBounded, context,
                ValueIntegerBounds{std::numeric_limits<std::int32_t>::min(),
                                   std::numeric_limits<std::int32_t>::max()});
  }

  if (name == "int64") {
    return make(InstructionIndex::AssertionNumberIntegerBounded, context,
                ValueIntegerBounds{std::numeric_limits<std::int64_t>::min(),
                                   std::numeric_limits<std::int64_t>::max()});
  }

  return make(InstructionIndex::AssertionNumberType, context,
              name == "float" ? ValueNumberType::Float
                              : ValueNumberType::Double);
}

auto compiler_draft3_validation_format(const KeywordContext &context,
                                       const Instructions &) -> Instructions {
  using Known = sourcemeta::core::SchemaVocabularies::Known;
  static constexpr auto UNSUPPORTED_DIALECT_MESSAGE{
      "The format assertion tweak not supported in this dialect"};

  const auto is_2019_09_format{
      context.vocabularies().contains(Known::JSON_SCHEMA_2019_09_FORMAT)};
  const auto is_2020_12_format_annotation{context.vocabularies().contains(
      Known::JSON_SCHEMA_2020_12_FORMAT_ANNOTATION)};
  const auto is_2020_12_format_assertion{context.vocabularies().contains(
      Known::JSON_SCHEMA_2020_12_FORMAT_ASSERTION)};

  const auto force_assertion{
      context.schema().is_object() &&
      context.schema().defines("x-format-assertion") &&
      context.schema().at("x-format-assertion").is_boolean() &&
      context.schema().at("x-format-assertion").to_boolean()};

  const auto assert_active{context.global.tweaks.format_assertion ||
                           force_assertion};
  if ((is_2019_09_format && assert_active) || is_2020_12_format_assertion ||
      (is_2020_12_format_annotation && assert_active)) {
    const auto &format{context.value()};
    if (!format.is_string()) {
      return {};
    }

    const auto &name{format.to_string()};

    // These are the only formats the OpenAPI 3.1 and 3.2 specifications
    // define themselves, and supporting any other entry of the Format
    // Registry they defer to is strictly optional
    // TODO: Consider asserting the numeric and string formats that the
    // OpenAPI Format Registry defines
    if (is_openapi_numeric_format(context, format)) {
      Instructions instructions{
          make_openapi_numeric_format_assertion(context, name)};

      // Unlike the string formats, the annotation cannot be grouped by the
      // instance type, as `ControlGroupWhenType` does not take a number
      if (annotations_enabled(context)) {
        instructions.push_back(make(InstructionIndex::AnnotationEmit, context,
                                    sourcemeta::core::JSON{context.value()}));
      }

      return instructions;
    }

    ValueStringType type;
    if (name == "date-time") {
      type = ValueStringType::DateTime;
    } else if (name == "date") {
      type = ValueStringType::Date;
    } else if (name == "time") {
      type = ValueStringType::Time;
    } else if (name == "duration") {
      type = ValueStringType::Duration;
    } else if (name == "email") {
      type = ValueStringType::Email;
    } else if (name == "idn-email") {
      type = ValueStringType::IDNEmail;
    } else if (name == "hostname") {
      type = ValueStringType::Hostname;
    } else if (name == "idn-hostname") {
      type = ValueStringType::IDNHostname;
    } else if (name == "ipv4") {
      type = ValueStringType::IPv4;
    } else if (name == "ipv6") {
      type = ValueStringType::IPv6;
    } else if (name == "uri") {
      type = ValueStringType::URI;
    } else if (name == "uri-reference") {
      type = ValueStringType::URIReference;
    } else if (name == "iri") {
      type = ValueStringType::IRI;
    } else if (name == "iri-reference") {
      type = ValueStringType::IRIReference;
    } else if (name == "uri-template") {
      type = ValueStringType::URITemplate;
    } else if (name == "json-pointer") {
      type = ValueStringType::JSONPointer;
    } else if (name == "relative-json-pointer") {
      type = ValueStringType::RelativeJSONPointer;
    } else if (name == "regex") {
      type = ValueStringType::Regex;
    } else if (name == "uuid") {
      type = ValueStringType::UUID;
    } else {
      return {};
    }

    Instructions instructions{
        make(InstructionIndex::AssertionStringType, context, type)};

    if (annotations_enabled(context)) {
      Instructions annotation_children{
          make(InstructionIndex::AnnotationEmit, context,
               sourcemeta::core::JSON{context.value()})};
      instructions.push_back(make(InstructionIndex::ControlGroupWhenType,
                                  context.relative(), ValueType::String,
                                  std::move(annotation_children)));
    }

    return instructions;
  }

  if (is_2019_09_format || is_2020_12_format_annotation) {
    if (!annotations_enabled(context)) {
      return {};
    }

    Instructions children{
        make(InstructionIndex::AnnotationEmit, context,
             sourcemeta::core::JSON{context.schema().at(context.keyword())})};

    // A format that applies to numbers cannot have its annotation grouped by
    // the instance type, so emit it the way the assertion path does rather
    // than collect it for strings alone
    if (is_openapi_numeric_format(context, context.value())) {
      return children;
    }

    return {make(InstructionIndex::ControlGroupWhenType, context.relative(),
                 ValueType::String, std::move(children))};
  }

  if (!assert_active) {
    return {};
  }

  const auto &format{context.value()};
  if (!format.is_string()) {
    return {};
  }

  const auto &name{format.to_string()};
  ValueStringType type;

  const auto draft3{is_draft3(context.vocabularies())};
  const auto draft4{is_draft4(context.vocabularies())};
  const auto draft6{is_draft6(context.vocabularies())};
  const auto draft7{is_draft7(context.vocabularies())};

  if (is_openapi_3_0(context.vocabularies())) {
    // OpenAPI 3.0 defines these four for numbers, so they cannot resolve to a
    // string logical type like the rest of this chain does
    if (name == "int32" || name == "int64" || name == "float" ||
        name == "double") {
      return {make_openapi_numeric_format_assertion(context, name)};
    }

    if (name == "date-time") {
      type = ValueStringType::DateTime;
    } else if (name == "date") {
      type = ValueStringType::Date;
    } else if (name == "byte") {
      type = ValueStringType::Base64;
    } else if (name == "email") {
      // The Schema Object is a subset of JSON Schema Draft Wright-00, whose
      // own formats apply alongside the ones the specification tabulates
      type = ValueStringType::Email;
    } else if (name == "hostname") {
      type = ValueStringType::Hostname;
    } else if (name == "ipv4") {
      type = ValueStringType::IPv4;
    } else if (name == "ipv6") {
      type = ValueStringType::IPv6;
    } else if (name == "uri") {
      type = ValueStringType::URI;
    } else {
      // `binary` stands for unencoded binary data, which a JSON string cannot
      // carry, and `password` is a hint to obscure the value, so neither of
      // the two remaining formats constrains the instance
      return {};
    }
  } else if (draft7) {
    if (name == "date-time") {
      type = ValueStringType::DateTime;
    } else if (name == "date") {
      type = ValueStringType::Date;
    } else if (name == "time") {
      type = ValueStringType::Time;
    } else if (name == "email") {
      type = ValueStringType::Email;
    } else if (name == "idn-email") {
      type = ValueStringType::IDNEmail;
    } else if (name == "hostname") {
      type = ValueStringType::Hostname;
    } else if (name == "idn-hostname") {
      type = ValueStringType::IDNHostname;
    } else if (name == "ipv4") {
      type = ValueStringType::IPv4;
    } else if (name == "ipv6") {
      type = ValueStringType::IPv6;
    } else if (name == "uri") {
      type = ValueStringType::URI;
    } else if (name == "uri-reference") {
      type = ValueStringType::URIReference;
    } else if (name == "uri-template") {
      type = ValueStringType::URITemplate;
    } else if (name == "json-pointer") {
      type = ValueStringType::JSONPointer;
    } else if (name == "relative-json-pointer") {
      type = ValueStringType::RelativeJSONPointer;
    } else if (name == "regex") {
      type = ValueStringType::Regex;
    } else if (name == "iri") {
      type = ValueStringType::IRI;
    } else if (name == "iri-reference") {
      type = ValueStringType::IRIReference;
    } else {
      return {};
    }
  } else if (draft4 || draft6) {
    if (name == "date-time") {
      type = ValueStringType::DateTime;
    } else if (name == "email") {
      type = ValueStringType::Email;
    } else if (name == "hostname") {
      type = ValueStringType::Hostname;
    } else if (name == "ipv4") {
      type = ValueStringType::IPv4;
    } else if (name == "ipv6") {
      type = ValueStringType::IPv6;
    } else if (name == "uri") {
      type = ValueStringType::URI;
    } else if (draft6 && name == "uri-reference") {
      type = ValueStringType::URIReference;
    } else if (draft6 && name == "uri-template") {
      type = ValueStringType::URITemplate;
    } else if (draft6 && name == "json-pointer") {
      type = ValueStringType::JSONPointer;
    } else {
      return {};
    }
  } else if (draft3) {
    if (name == "date-time") {
      type = ValueStringType::DateTime;
    } else if (name == "email") {
      type = ValueStringType::Email;
    } else if (name == "host-name") {
      type = ValueStringType::Hostname;
    } else if (name == "ip-address") {
      type = ValueStringType::IPv4;
    } else if (name == "ipv6") {
      type = ValueStringType::IPv6;
    } else if (name == "uri") {
      type = ValueStringType::URI;
    } else if (name == "date") {
      type = ValueStringType::Date;
    } else if (name == "time") {
      type = ValueStringType::PartialTime;
    } else if (name == "utc-millisec") {
      // TODO: Support this old format, even though not even the official test
      // suite covers it
      throw_keyword_error(
          context,
          "The \"utc-millisec\" format is not supported in assertion mode yet");
    } else if (name == "regex") {
      type = ValueStringType::Regex;
    } else if (name == "color") {
      type = ValueStringType::Color;
    } else if (name == "style") {
      // TODO: Support this old format, even though not even the official test
      // suite covers it
      throw_keyword_error(
          context,
          "The \"style\" format is not supported in assertion mode yet");
    } else if (name == "phone") {
      // TODO: Support this old format, even though not even the official test
      // suite covers it
      throw_keyword_error(
          context,
          "The \"phone\" format is not supported in assertion mode yet");
    } else {
      return {};
    }
  } else {
    throw_keyword_error(context, UNSUPPORTED_DIALECT_MESSAGE);
  }

  return {make(InstructionIndex::AssertionStringType, context, type)};
}

} // namespace sourcemeta::blaze
#endif
