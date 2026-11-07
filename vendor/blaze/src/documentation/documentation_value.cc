#include "internal.h"

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/regex.h>

#include <algorithm>   // std::sort, std::any_of, std::find, std::max
#include <cassert>     // assert
#include <cstddef>     // std::size_t
#include <cstdint>     // std::int64_t, std::uint8_t, std::uint32_t
#include <map>         // std::map
#include <optional>    // std::optional
#include <set>         // std::set
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move, std::pair
#include <vector>      // std::vector

// Turning one subschema into one value of the table format. Every rule it
// states must hold for the data the schema accepts, and what cannot be stated
// exactly is refused rather than guessed at.

namespace {

using sourcemeta::blaze::also;
using sourcemeta::blaze::array_facts;
using sourcemeta::blaze::condition;
using sourcemeta::blaze::copy_if_present;
using sourcemeta::blaze::defines;
using sourcemeta::blaze::describe_subschema;
using sourcemeta::blaze::DocumentationContext;
using sourcemeta::blaze::fail;
using sourcemeta::blaze::make_value;
using sourcemeta::blaze::MAXIMUM_DEPTH;
using sourcemeta::blaze::number_facts;
using sourcemeta::blaze::object_facts;
using sourcemeta::blaze::Rank;
using sourcemeta::blaze::rank_at;
using sourcemeta::blaze::string_facts;
using sourcemeta::core::JSON;
using sourcemeta::core::Pointer;

// Whether a value, as the format describes it, accepts null.
auto accepts_null(const JSON &value) -> bool {
  if (value.defines("nullable")) {
    return true;
  }

  const auto &kind{value.at("kind").to_string()};
  if (kind == "null" || kind == "any" || kind == "ref" || kind == "external") {
    // A shape or another document might accept null, and this cannot tell.
    return true;
  }

  if (kind == "enum") {
    const auto &values{value.at("values")};
    return std::any_of(values.as_array().cbegin(), values.as_array().cend(),
                       [](const auto &entry) { return entry.is_null(); });
  }

  if (value.defines("otherTypesAllowed")) {
    return true;
  }

  if (kind == "choice") {
    const auto &options{value.at("options")};
    return std::any_of(options.as_array().cbegin(), options.as_array().cend(),
                       [](const auto &option) { return accepts_null(option); });
  }

  return false;
}

// `not` forbids, and two prohibitions both hold: not this and not that is not
// (this or that).
auto forbid(JSON &value, JSON &&forbidden) -> void {
  if (!value.defines("not")) {
    value.assign("not", std::move(forbidden));
    return;
  }

  auto options{JSON::make_array()};
  options.push_back(value.at("not"));
  options.push_back(std::move(forbidden));
  auto choice{make_value("choice")};
  choice.assign("options", std::move(options));
  choice.assign("overlap", JSON{true});
  value.assign("not", std::move(choice));
}

auto is_type_name(const JSON::String &name) -> bool {
  return name == "string" || name == "number" || name == "integer" ||
         name == "boolean" || name == "object" || name == "array" ||
         name == "null";
}

// The types the KEYWORDS of a subschema are about, when it does not say.
// Whether a subschema uses any of the given keywords.
auto defines_any(const JSON &subschema,
                 const std::vector<std::string> &keywords) -> bool {
  return std::any_of(keywords.cbegin(), keywords.cend(),
                     [&subschema](const auto &keyword) {
                       return defines(subschema, keyword);
                     });
}

auto inferred_kinds(const JSON &subschema, const Rank rank)
    -> std::vector<std::string> {
  std::vector<std::string> string_keywords{"minLength", "maxLength", "pattern"};
  std::vector<std::string> number_keywords{
      "minimum", "maximum", "exclusiveMinimum", "exclusiveMaximum"};
  std::vector<std::string> object_keywords{"properties", "patternProperties",
                                           "additionalProperties"};
  std::vector<std::string> array_keywords{
      "items", "additionalItems", "minItems", "maxItems", "uniqueItems"};
  // Draft 3 marks a property required on the property, with a boolean that
  // says nothing about the kind of the value.
  if (rank == Rank::Draft3) {
    number_keywords.emplace_back("divisibleBy");
  } else {
    number_keywords.emplace_back("multipleOf");
    object_keywords.emplace_back("required");
    object_keywords.emplace_back("minProperties");
    object_keywords.emplace_back("maxProperties");
  }

  if (rank >= Rank::Draft6) {
    object_keywords.emplace_back("propertyNames");
    array_keywords.emplace_back("contains");
  }

  if (rank >= Rank::Draft7) {
    string_keywords.emplace_back("contentMediaType");
    string_keywords.emplace_back("contentEncoding");
  }

  if (rank >= Rank::Draft2019) {
    string_keywords.emplace_back("contentSchema");
    object_keywords.emplace_back("dependentRequired");
    object_keywords.emplace_back("dependentSchemas");
    object_keywords.emplace_back("unevaluatedProperties");
    array_keywords.emplace_back("minContains");
    array_keywords.emplace_back("maxContains");
    array_keywords.emplace_back("unevaluatedItems");
  }

  object_keywords.emplace_back("dependencies");

  if (rank >= Rank::Draft2020) {
    array_keywords.emplace_back("prefixItems");
  }

  std::vector<std::string> result;
  // A label from the source language is about text, unless it names a way of
  // writing a number.
  static const std::vector<std::string> NUMBER_FORMATS{
      "int32", "int64", "float", "double", "integer", "number"};
  const auto labelled_number{
      defines(subschema, "format") && subschema.at("format").is_string() &&
      std::find(NUMBER_FORMATS.cbegin(), NUMBER_FORMATS.cend(),
                subschema.at("format").to_string()) != NUMBER_FORMATS.cend()};
  const auto labelled_text{defines(subschema, "format") &&
                           subschema.at("format").is_string() &&
                           !labelled_number};

  if (defines_any(subschema, string_keywords) || labelled_text) {
    result.emplace_back("string");
  }

  if (defines_any(subschema, number_keywords) || labelled_number) {
    result.emplace_back("number");
  }

  if (defines_any(subschema, object_keywords)) {
    result.emplace_back("object");
  }

  if (defines_any(subschema, array_keywords)) {
    result.emplace_back("array");
  }

  return result;
}

auto fill_kind(DocumentationContext &context, JSON &value,
               const JSON &subschema, const Pointer &pointer) -> void {
  const auto &kind{value.at("kind").to_string()};
  if (kind == "string") {
    string_facts(context, value, subschema, pointer);
  } else if (kind == "number" || kind == "integer") {
    number_facts(value, subschema, context.rank);
  } else if (kind == "object") {
    object_facts(context, value, subschema, pointer);
  } else if (kind == "array") {
    array_facts(context, value, subschema, pointer);
  }
}

auto annotations(JSON &value, const JSON &subschema, const Rank rank) -> void {
  copy_if_present(subschema, "title", value, "title");
  copy_if_present(subschema, "description", value, "description");
  copy_if_present(subschema, "default", value, "default");
  if (rank >= Rank::Draft6 && defines(subschema, "examples") &&
      subschema.at("examples").is_array() &&
      !subschema.at("examples").empty()) {
    value.assign("examples", subschema.at("examples"));
  }

  if (rank >= Rank::Draft2019 && defines(subschema, "deprecated") &&
      subschema.at("deprecated").is_boolean() &&
      subschema.at("deprecated").to_boolean()) {
    value.assign("deprecated", JSON{true});
  }

  const auto read_only{rank >= Rank::Draft7 && defines(subschema, "readOnly") &&
                       subschema.at("readOnly").is_boolean() &&
                       subschema.at("readOnly").to_boolean()};
  const auto write_only{rank >= Rank::Draft7 &&
                        defines(subschema, "writeOnly") &&
                        subschema.at("writeOnly").is_boolean() &&
                        subschema.at("writeOnly").to_boolean()};
  // Both at once says nothing a reader can act on, so it says nothing here.
  if (read_only && !write_only) {
    value.assign("access", JSON{std::string{"read"}});
  } else if (write_only && !read_only) {
    value.assign("access", JSON{std::string{"write"}});
  }
}

// A form that is only null says the value may be null instead.
auto only_null(const JSON &option) -> bool {
  return option.at("kind").to_string() == "null" && option.size() == 1;
}

auto branches(DocumentationContext &context, JSON &value, const JSON &subschema,
              const Pointer &pointer, const std::string &keyword,
              const bool overlap) -> void {
  if (!defines(subschema, keyword)) {
    return;
  }

  const auto &members{subschema.at(keyword)};
  assert(members.is_array() && !members.empty());

  std::vector<JSON> options;
  options.reserve(members.array_size());
  for (std::size_t index = 0; index < members.array_size(); index++) {
    options.emplace_back(describe_subschema(
        context, members.at(index), pointer.concat(keyword).concat(index)));
  }

  if (options.size() > 1) {
    const auto nulls{
        std::count_if(options.cbegin(), options.cend(), only_null)};
    const auto others{std::count_if(
        options.cbegin(), options.cend(), [&](const auto &option) {
          return !only_null(option) && accepts_null(option);
        })};
    if (nulls == 1 && others == 0) {
      options.erase(std::remove_if(options.begin(), options.end(), only_null),
                    options.end());
      value.assign("nullable", JSON{true});
    }
  }

  if (options.empty()) {
    return;
  }

  if (options.size() == 1) {
    // One form is not a choice: it is simply another thing that must hold.
    also(value, std::move(options.front()));
    return;
  }

  auto choice{make_value("choice")};
  auto entries{JSON::make_array()};
  for (auto &option : options) {
    entries.push_back(std::move(option));
  }

  choice.assign("options", std::move(entries));
  if (overlap) {
    choice.assign("overlap", JSON{true});
  }

  also(value, std::move(choice));
}

auto conditions(DocumentationContext &context, JSON &value,
                const JSON &subschema, const Pointer &pointer) -> void {
  if (!defines(subschema, "if") || context.rank < Rank::Draft7) {
    return;
  }

  if (!defines(subschema, "then") && !defines(subschema, "else")) {
    // Neither branch says anything, so neither does the condition.
    return;
  }

  auto entry{JSON::make_object()};
  entry.assign("when", describe_subschema(context, subschema.at("if"),
                                          pointer.concat("if")));
  if (defines(subschema, "then")) {
    entry.assign("then", describe_subschema(context, subschema.at("then"),
                                            pointer.concat("then")));
  }

  if (defines(subschema, "else")) {
    entry.assign("otherwise", describe_subschema(context, subschema.at("else"),
                                                 pointer.concat("else")));
  }

  condition(value, std::move(entry));
}

/// Inside one subschema, the conversion is one level deeper, and reads that
// subschema in the dialect in effect where it sits.
class Visit {
public:
  Visit(DocumentationContext &context, const Pointer &pointer)
      : context_{context}, previous_{context.rank} {
    this->context_.depth += 1;
    if (this->context_.depth > MAXIMUM_DEPTH) {
      fail("This schema nests deeper than this module describes", pointer);
    }

    this->context_.rank = rank_at(context, pointer);
  }

  ~Visit() {
    this->context_.depth -= 1;
    this->context_.rank = this->previous_;
  }

  Visit(const Visit &) = delete;
  auto operator=(const Visit &) -> Visit & = delete;
  Visit(Visit &&) = delete;
  auto operator=(Visit &&) -> Visit & = delete;

private:
  DocumentationContext &context_;
  Rank previous_;
};

// Whether a Draft 3 `type` or `disallow` names every type, with `any`.
// Draft 3 lets a type name be anything, and a name that is not a JSON type is
// one any value matches, like `any` itself.
auto means_any(const JSON &entry) -> bool {
  return entry.is_string() &&
         (entry.to_string() == "any" || !is_type_name(entry.to_string()));
}

auto names_any(const JSON &types) -> bool {
  return means_any(types) ||
         (types.is_array() && std::any_of(types.as_array().cbegin(),
                                          types.as_array().cend(), means_any));
}

// One entry of a Draft 3 list of types, as a form a value may take.
auto type_option(DocumentationContext &context, const JSON &entry,
                 const Pointer &where) -> JSON {
  if (entry.is_object()) {
    return describe_subschema(context, entry, where);
  }

  assert(entry.is_string());
  return make_value(means_any(entry) ? "any" : entry.to_string());
}

// Whether a Draft 3 `type` or `disallow` holds a whole schema.
auto holds_schemas(const JSON &types) -> bool {
  return types.is_array() &&
         std::any_of(types.as_array().cbegin(), types.as_array().cend(),
                     [](const auto &entry) { return entry.is_object(); });
}

// A Draft 3 list of types, which may hold whole schemas as well as names, as
// the forms a value may take.
auto type_options(DocumentationContext &context, const JSON &types,
                  const Pointer &pointer, const std::string &keyword) -> JSON {
  std::vector<JSON> options;
  if (types.is_array()) {
    for (std::size_t index = 0; index < types.array_size(); index++) {
      options.push_back(type_option(context, types.at(index),
                                    pointer.concat(keyword).concat(index)));
    }
  } else {
    options.push_back(type_option(context, types, pointer.concat(keyword)));
  }

  if (options.size() == 1) {
    return std::move(options.front());
  }

  auto entries{JSON::make_array()};
  for (auto &option : options) {
    entries.push_back(std::move(option));
  }

  auto choice{make_value("choice")};
  choice.assign("options", std::move(entries));
  choice.assign("overlap", JSON{true});
  return choice;
}

// The keywords Draft 3 has and later drafts spell otherwise: `extends` for
// `allOf`, `disallow` for `not`, and a `type` that holds whole schemas for
// `anyOf`.
auto draft3_facts(DocumentationContext &context, JSON &value,
                  const JSON &subschema, const Pointer &pointer) -> void {
  if (defines(subschema, "type") && holds_schemas(subschema.at("type")) &&
      !names_any(subschema.at("type"))) {
    also(value, type_options(context, subschema.at("type"), pointer, "type"));
  }

  if (defines(subschema, "extends")) {
    const auto &extends{subschema.at("extends")};
    if (extends.is_array()) {
      for (std::size_t index = 0; index < extends.array_size(); index++) {
        also(value,
             describe_subschema(context, extends.at(index),
                                pointer.concat("extends").concat(index)));
      }
    } else {
      also(value,
           describe_subschema(context, extends, pointer.concat("extends")));
    }
  }

  if (defines(subschema, "disallow")) {
    forbid(value, type_options(context, subschema.at("disallow"), pointer,
                               "disallow"));
  }
}

} // namespace

namespace sourcemeta::blaze {

auto describe_subschema(DocumentationContext &context, const JSON &subschema,
                        const Pointer &pointer) -> JSON {
  const Visit visit{context, pointer};
  if (subschema.is_boolean()) {
    return make_value(subschema.to_boolean() ? "any" : "never");
  }

  assert(subschema.is_object());
  // A keyword whose vocabulary the meta-schema leaves out says nothing here, so
  // the subschema is read without it.
  const auto ignored{
      context.ignored.find(sourcemeta::core::to_string(pointer))};
  if (ignored != context.ignored.cend() &&
      std::any_of(ignored->second.cbegin(), ignored->second.cend(),
                  [&subschema](const auto &keyword) {
                    return subschema.defines(keyword);
                  })) {
    auto usable{JSON::make_object()};
    for (const auto &entry : subschema.as_object()) {
      if (!ignored->second.contains(entry.first)) {
        usable.assign(entry.first, entry.second);
      }
    }

    return describe_subschema(context, usable, pointer);
  }

  auto references{references_of(context, subschema, pointer)};
  // Before 2019-09, a reference stands alone: its siblings say nothing.
  if (defines(subschema, "$ref") && context.rank < Rank::Draft2019) {
    return std::move(references.front());
  }

  // A Draft 3 `type` may allow every type, which says nothing, or hold whole
  // schemas, which makes it one of several forms rather than a list of names.
  const auto draft3_type{
      context.rank == Rank::Draft3 && defines(subschema, "type") &&
      (names_any(subschema.at("type")) || holds_schemas(subschema.at("type")))};
  if (context.rank == Rank::Draft3 && defines(subschema, "type") &&
      subschema.at("type").is_array() && subschema.at("type").empty()) {
    // Draft 3 allows a list of no types, which no value is.
    return make_value("never");
  }

  JSON value{JSON::make_object()};
  std::vector<std::string> kinds;
  if (defines(subschema, "type") && !draft3_type) {
    const auto &type{subschema.at("type")};
    if (type.is_string()) {
      assert(is_type_name(type.to_string()));
      kinds.push_back(type.to_string());
    } else {
      assert(type.is_array() && !type.empty());
      std::set<std::string> seen;
      for (const auto &entry : type.as_array()) {
        assert(entry.is_string() && is_type_name(entry.to_string()));
        [[maybe_unused]] const auto unique{
            seen.insert(entry.to_string()).second};
        assert(unique);
        kinds.push_back(entry.to_string());
      }
    }
  }

  const auto has_enumeration{
      defines(subschema, "enum") ||
      (defines(subschema, "const") && context.rank >= Rank::Draft6)};
  // Saying null is also allowed is only true when nothing beside the type can
  // forbid it. Otherwise null stays one of the forms, and the rules beside it
  // narrow the whole value as they do for every other form.
  static const std::vector<std::string> NARROWING{"allOf",
                                                  "anyOf",
                                                  "oneOf",
                                                  "not",
                                                  "$ref",
                                                  "if",
                                                  "enum",
                                                  "const",
                                                  "extends",
                                                  "disallow",
                                                  "$dynamicRef",
                                                  "$recursiveRef",
                                                  "unevaluatedProperties",
                                                  "unevaluatedItems"};
  auto can_narrow{false};
  for (const auto &keyword : NARROWING) {
    if (subschema.defines(keyword)) {
      can_narrow = true;
    }
  }

  auto nullable{false};
  if (kinds.size() > 1 && !can_narrow) {
    const auto null{std::find(kinds.begin(), kinds.end(), "null")};
    if (null != kinds.end()) {
      kinds.erase(null);
      nullable = true;
    }
  }

  if (has_enumeration) {
    // A fixed set of values, keeping only those the KEYWORDS beside it accept,
    // since a value the schema rejects must not be shown as allowed.
    std::vector<JSON> allowed;
    if (defines(subschema, "enum")) {
      const auto &listed{subschema.at("enum")};
      // From Draft 6 the meta-schema allows an empty list, which no value is
      // in.
      assert(listed.is_array() &&
             (!listed.empty() || context.rank >= Rank::Draft6));

      allowed.reserve(listed.array_size());
      for (const auto &entry : listed.as_array()) {
        allowed.push_back(entry);
      }
    }

    if (defines(subschema, "const") && context.rank >= Rank::Draft6) {
      const auto &only{subschema.at("const")};
      if (!defines(subschema, "enum")) {
        allowed.push_back(only);
      } else {
        // Both hold, so only a value that is in the list and is the constant
        // is left.
        std::vector<JSON> both;
        for (const auto &entry : allowed) {
          if (entry == only) {
            both.push_back(entry);
          }
        }

        allowed = both;
      }
    }

    std::vector<JSON> kept;
    kept.reserve(allowed.size());
    auto certain{true};
    for (const auto &entry : allowed) {
      const auto verdict{accepted_by(entry, subschema, kinds, context.rank)};
      if (verdict == Verdict::No) {
        continue;
      }

      if (verdict == Verdict::Unknown) {
        certain = false;
      }

      kept.push_back(entry);
    }

    if (kept.empty()) {
      value = make_value("never");
    } else {
      value = make_value("enum");
      auto values{JSON::make_array()};
      for (auto &entry : kept) {
        values.push_back(std::move(entry));
      }

      value.assign("values", std::move(values));
      // What could not be DECIDED about here is said as a rule of its own, so
      // that nothing the schema requires is lost.
      if (!certain) {
        auto rest{JSON{subschema}};
        rest.erase(JSON::String{"enum"});
        rest.erase(JSON::String{"const"});
        rest.erase(JSON::String{"title"});
        rest.erase(JSON::String{"description"});
        rest.erase(JSON::String{"default"});
        rest.erase(JSON::String{"examples"});
        auto beside{describe_subschema(context, rest, pointer)};
        if (!(beside.at("kind").to_string() == "any" && beside.size() == 1)) {
          also(value, std::move(beside));
        }
      }
    }

    // Whatever the references beside the values say is in the rule above, if
    // anything was left to say.
    references.clear();
    nullable = false;
  } else if (kinds.empty()) {
    const auto inferred{inferred_kinds(subschema, context.rank)};
    if (inferred.empty()) {
      value = make_value("any");
    } else if (inferred.size() == 1) {
      value = make_value(inferred.front());
      fill_kind(context, value, subschema, pointer);
      value.assign("otherTypesAllowed", JSON{true});
    } else {
      auto options{JSON::make_array()};
      for (const auto &kind : inferred) {
        auto option{make_value(kind)};
        fill_kind(context, option, subschema, pointer);
        options.push_back(std::move(option));
      }

      value = make_value("choice");
      value.assign("options", std::move(options));
      value.assign("overlap", JSON{true});
      value.assign("otherTypesAllowed", JSON{true});
    }
  } else if (kinds.size() == 1) {
    value = make_value(kinds.front());
    fill_kind(context, value, subschema, pointer);
  } else {
    auto options{JSON::make_array()};
    for (const auto &kind : kinds) {
      auto option{make_value(kind)};
      fill_kind(context, option, subschema, pointer);
      options.push_back(std::move(option));
    }

    value = make_value("choice");
    value.assign("options", std::move(options));
    // A whole number is a number too, so those two forms overlap.
    if (std::find(kinds.cbegin(), kinds.cend(), "number") != kinds.cend() &&
        std::find(kinds.cbegin(), kinds.cend(), "integer") != kinds.cend()) {
      value.assign("overlap", JSON{true});
    }
  }

  if (nullable) {
    value.assign("nullable", JSON{true});
  }

  for (auto &reference : references) {
    if (value.at("kind").to_string() == "any" && value.size() == 1) {
      value = std::move(reference);
    } else {
      also(value, std::move(reference));
    }
  }

  if (context.rank == Rank::Draft3) {
    draft3_facts(context, value, subschema, pointer);
  } else {
    if (defines(subschema, "allOf")) {
      const auto &members{subschema.at("allOf")};
      assert(members.is_array() && !members.empty());

      // Every member holds, so each one is another thing the data must match.
      for (std::size_t index = 0; index < members.array_size(); index++) {
        also(value, describe_subschema(context, members.at(index),
                                       pointer.concat("allOf").concat(index)));
      }
    }

    branches(context, value, subschema, pointer, "anyOf", true);
    branches(context, value, subschema, pointer, "oneOf", false);
    if (defines(subschema, "not")) {
      forbid(value, describe_subschema(context, subschema.at("not"),
                                       pointer.concat("not")));
    }
  }

  conditions(context, value, subschema, pointer);
  annotations(value, subschema, context.rank);
  return value;
}

} // namespace sourcemeta::blaze
