#include "jsonld_algorithms.h"
#include "jsonld_keywords.h"

#include <sourcemeta/core/uri.h>

#include <algorithm> // std::ranges::sort
#include <cstddef>   // std::size_t, std::ptrdiff_t
#include <optional>  // std::optional
#include <utility>   // std::move, std::pair
#include <vector>    // std::vector

namespace sourcemeta::core {

namespace {

auto into_array(JSON &&value) -> JSON {
  if (value.is_array()) {
    return std::move(value);
  }
  auto result{JSON::make_array()};
  result.push_back(std::move(value));
  return result;
}

// The entries of an object in sorted key order, which is the order expansion
// uses so that values merged from several keys are deterministic. The keys and
// values are referenced from the object (which must outlive the result), never
// copied.
auto sorted_entries(const JSON &object)
    -> std::vector<std::pair<const JSON::String *, const JSON *>> {
  std::vector<std::pair<const JSON::String *, const JSON *>> entries;
  for (const auto &entry : object.as_object()) {
    entries.emplace_back(&entry.first, &entry.second);
  }
  std::ranges::sort(entries, [](const auto &left, const auto &right) -> bool {
    return *left.first < *right.first;
  });
  return entries;
}

// Append the values, which must be an array, into the array stored at the given
// key, creating it if absent.
auto merge(JSON &object, const JSON::StringView name, JSON &&values) -> void {
  if (object.defines(name)) {
    for (auto &item : values.as_array()) {
      object.at(name).push_back(item);
    }
  } else {
    object.assign(name, std::move(values));
  }
}

// Whether an expanded member is one that the position it landed in forbids
auto is_not_node_object(const JSON &item) -> bool {
  return !item.is_object() || item.defines(KEYWORD_VALUE, KEYWORD_VALUE_HASH) ||
         item.defines(KEYWORD_LIST, KEYWORD_LIST_HASH) ||
         item.defines(KEYWORD_SET, KEYWORD_SET_HASH);
}

auto is_value_object(const JSON &item) -> bool {
  return item.is_object() && item.defines(KEYWORD_VALUE, KEYWORD_VALUE_HASH);
}

auto is_value_or_list_object(const JSON &item) -> bool {
  return item.is_object() && (item.defines(KEYWORD_VALUE, KEYWORD_VALUE_HASH) ||
                              item.defines(KEYWORD_LIST, KEYWORD_LIST_HASH));
}

auto is_list_object(const JSON &item) -> bool {
  return item.is_object() && item.defines(KEYWORD_LIST, KEYWORD_LIST_HASH);
}

using ForbiddenMember = bool (*)(const JSON &);

// Whether an expansion holds a member its position forbids. A null expansion
// carried nothing over, so it is never the one at fault
auto holds_forbidden_member(const JSON &expanded,
                            const ForbiddenMember forbidden) -> bool {
  if (expanded.is_null()) {
    return false;
  }

  if (expanded.is_array()) {
    for (const auto &item : expanded.as_array()) {
      if (forbidden(item)) {
        return true;
      }
    }

    return false;
  }

  return forbidden(expanded);
}

// Declared ahead of the member tracer below, which needs it to reproduce the
// context an offending member was originally expanded under
auto apply_scoped_context(ExpansionState &state, ActiveContext &context,
                          const TermDefinition &definition) -> void;

// The position the offending member was written at, found by descending
// through the arrays and set objects that flatten into the expansion.
// Expansion does not retain which input each expanded member came from, so the
// members are expanded again to find it, which only ever happens on the way
// out of an error. Locating a member must not change what is reported, so
// whatever that re-expansion runs into costs no more than the precision of the
// position
auto forbidden_member_pointer(
    ExpansionState &state, ActiveContext &active_context,
    const std::optional<JSON::String> &active_property, const JSON &member,
    const WeakPointer &pointer, const ForbiddenMember forbidden) -> Pointer {
  try {
    if (member.is_array()) {
      std::size_t index{0};
      for (const auto &item : member.as_array()) {
        const WeakPointer item_pointer{pointer.concat(index)};
        if (holds_forbidden_member(expand(state, active_context,
                                          active_property, item, item_pointer),
                                   forbidden)) {
          return forbidden_member_pointer(state, active_context,
                                          active_property, item, item_pointer,
                                          forbidden);
        }
        index += 1;
      }
    } else if (member.is_object()) {
      // A set object hands its members to the enclosing position, and the
      // context of the property or of the member itself may be what names the
      // keyword, so both are layered on before the keys are read, the way they
      // are when the member is expanded
      ActiveContext effective{active_context};
      if (active_property.has_value()) {
        const auto definition{
            active_context.terms.find(active_property.value())};
        if (definition != active_context.terms.cend() &&
            definition->second.context.has_value()) {
          effective.previous = nullptr;
          apply_scoped_context(state, effective, definition->second);
        }
      }
      if (member.defines(KEYWORD_CONTEXT, KEYWORD_CONTEXT_HASH)) {
        process_context(state, effective,
                        member.at(KEYWORD_CONTEXT, KEYWORD_CONTEXT_HASH),
                        pointer.concat(keyword_context()));
      }
      for (const auto &entry : member.as_object()) {
        const auto expanded_key{expand_iri(state, effective, entry.first, false,
                                           true, nullptr, nullptr,
                                           EMPTY_WEAK_POINTER)};
        if (expanded_key.has_value() && expanded_key.value() == KEYWORD_SET) {
          return forbidden_member_pointer(
              state, effective, active_property, entry.second,
              pointer.concat(entry.first), forbidden);
        }
      }
    }
  } catch (const JSONLDError &) {
    // The enclosing position stands when the member cannot be narrowed down
    return to_pointer(pointer);
  }

  return to_pointer(pointer);
}

auto container_includes(const TermDefinition *const definition,
                        const JSON::StringView name) -> bool {
  if (definition == nullptr) {
    return false;
  }
  for (const auto &entry : definition->container) {
    if (entry == name) {
      return true;
    }
  }
  return false;
}

// Expand a single @type value against the context that preceded type-scoped
// processing.
auto expand_type(ExpansionState &state, const ActiveContext &type_context,
                 const JSON &item) -> JSON {
  auto context{type_context};
  const auto type{expand_iri(state, context, item.to_string(), true, true,
                             nullptr, nullptr, EMPTY_WEAK_POINTER)};
  return type.has_value() ? JSON{type.value()} : JSON{nullptr};
}

// The entry of the input map that expanded into the given key of the result.
// Value objects and set or list objects are validated once expanded, while the
// reported position belongs to the input document, where a keyword may appear
// under an alias: "Within node objects, value objects, graph objects, list
// objects, set objects, and nested properties keyword aliases MAY be used
// instead of the corresponding keyword" (JSON-LD 1.1 Section 9.16)
auto offending_entry(ExpansionState &state, ActiveContext &active_context,
                     const JSON &element, const JSON::StringView name)
    -> const JSON::String * {
  const JSON::String *result{nullptr};
  for (const auto &entry : sorted_entries(element)) {
    const auto expanded{expand_iri(state, active_context, *entry.first, false,
                                   true, nullptr, nullptr, EMPTY_WEAK_POINTER)};
    if (expanded.has_value() && expanded.value() == name) {
      // Several keys may reach the same name, and what they carry is merged
      // before it is inspected, so none of them can be held to account for it
      if (result != nullptr) {
        return nullptr;
      }

      result = entry.first;
    }
  }

  return result;
}

// The input locations of the members that expanded into the keywords whose
// combination is only judged once the whole element is expanded. A keyword may
// arrive from a nested property, whose object sits elsewhere in the input, so
// where it came from cannot be recovered from the element alone
struct KeywordOrigins {
  std::optional<WeakPointer> value;
  std::optional<WeakPointer> type;
  std::optional<WeakPointer> collection;
};

// Where a keyword the element carries was written, or the element itself when
// nothing recorded it
auto origin_pointer(const std::optional<WeakPointer> &origin,
                    const WeakPointer &pointer) -> Pointer {
  return to_pointer(origin.has_value() ? origin.value() : pointer);
}

// Locate a violation at the entry that caused it, falling back to where
// whatever contributed that entry was written when it was merged in from a
// nested map instead
auto offending_pointer(ExpansionState &state, ActiveContext &active_context,
                       const JSON &element, const WeakPointer &pointer,
                       const JSON::StringView name,
                       const std::optional<WeakPointer> &origin = std::nullopt)
    -> Pointer {
  const auto *const entry{
      offending_entry(state, active_context, element, name)};
  if (entry == nullptr) {
    return origin_pointer(origin, pointer);
  }

  auto result{to_pointer(pointer)};
  result.push_back(*entry);
  return result;
}

// Process a term's own context over the given one, reporting what it says
// wrong where the context was written rather than wherever the term came into
// use. A context whose entries the input does not spell out has no position of
// its own to point into, so everything it says wrong collapses to the
// reference that brought it
auto process_deferred_scoped_context(ExpansionState &state,
                                     ActiveContext &context,
                                     const TermDefinition &definition,
                                     const bool propagate) -> void {
  const ContextBaseScope base{state, definition.context_base};
  // A definition that came from elsewhere restores that origin, so the terms
  // its context defines in turn are located the same way
  const ForeignContextScope origin{
      state, definition.context_authored
                 ? std::nullopt
                 : std::optional<Pointer>{definition.context_location}};
  try {
    process_context(state, context, definition.context.value(),
                    to_weak_pointer(definition.context_location), propagate);
  } catch (const JSONLDError &error) {
    if (definition.context_authored) {
      throw;
    }
    throw JSONLDError(error.what(), definition.context_location);
  }
}

// Layer a property-scoped context over the given one, which overrides
// protected terms (JSON-LD 1.1 API Section 5.1.2 step 8)
auto apply_scoped_context(ExpansionState &state, ActiveContext &context,
                          const TermDefinition &definition) -> void {
  const ProtectedOverrideScope override{state.protected_override};
  process_deferred_scoped_context(state, context, definition, true);
}

// Whether a nested object carries a value entry, which the nesting forbids,
// under a keyword alias too (JSON-LD 1.1 API Section 5.1.2 step 14.2.1)
auto nest_defines_value(ExpansionState &state, ActiveContext &active_context,
                        const JSON &nested) -> bool {
  for (const auto &entry : nested.as_object()) {
    const auto expanded{expand_iri(state, active_context, entry.first, false,
                                   true, nullptr, nullptr, EMPTY_WEAK_POINTER)};
    if (expanded.has_value() && expanded.value() == KEYWORD_VALUE) {
      return true;
    }
  }

  return false;
}

// Expand the direct (and deferred @nest) entries of a map into the result,
// mutating it in place. Mutually recursive with expand_object.
auto expand_entries(ExpansionState &state, ActiveContext &active_context,
                    const ActiveContext &type_context, JSON &result,
                    KeywordOrigins &origins,
                    const std::optional<JSON::String> &active_property,
                    const JSON &source, const WeakPointer &source_pointer)
    -> void;

// Expand a map element: the node-object (and value-object) branch of the
// Expansion algorithm, factored out of expand() below.
auto expand_object(ExpansionState &state, ActiveContext active_context,
                   const std::optional<JSON::String> &active_property,
                   const JSON &element, const WeakPointer &pointer) -> JSON {
  auto result{JSON::make_object()};

  // @type values are expanded against the context before type-scoped contexts
  // are applied.
  const ActiveContext type_context{active_context};

  // Type-scoped contexts. The entries that bear a type are visited in
  // lexicographical key order and the values of each are ordered within it, so
  // which type's context prevails follows the input keys rather than one
  // ordering across all of them. The values are referenced from the input
  // element, never copied (JSON-LD 1.1 API Section 5.1.2 step 11)
  std::vector<JSON::StringView> type_values;
  for (const auto &[key, value] : sorted_entries(element)) {
    const auto expanded{expand_iri(state, active_context, *key, false, true,
                                   nullptr, nullptr, EMPTY_WEAK_POINTER)};
    if (!expanded.has_value() || expanded.value() != KEYWORD_TYPE) {
      continue;
    }
    const auto entry_first{static_cast<std::ptrdiff_t>(type_values.size())};
    if (value->is_array()) {
      for (const auto &item : value->as_array()) {
        if (item.is_string()) {
          type_values.push_back(item.to_string());
        }
      }
    } else if (value->is_string()) {
      type_values.push_back(value->to_string());
    }
    std::ranges::sort(type_values.begin() + entry_first, type_values.end());
  }
  for (const auto &type : type_values) {
    // Each type-scoped context is resolved against the context that preceded
    // type-scoped processing, so one type's context cannot hide another's.
    const auto definition{type_context.terms.find(type)};
    if (definition != type_context.terms.cend() &&
        definition->second.context.has_value()) {
      process_deferred_scoped_context(state, active_context, definition->second,
                                      false);
    }
  }

  KeywordOrigins origins;
  expand_entries(state, active_context, type_context, result, origins,
                 active_property, element, pointer);

  // An empty reverse map carries no information.
  if (const auto *reverse{result.try_at(KEYWORD_REVERSE, KEYWORD_REVERSE_HASH)};
      reverse != nullptr && reverse->empty()) {
    result.erase(KEYWORD_REVERSE);
  }

  // Post-processing (JSON-LD 1.1 API Section 5.1.2)
  if (const auto *value_entry{
          result.try_at(KEYWORD_VALUE, KEYWORD_VALUE_HASH)}) {
    const JSON *const type{result.try_at(KEYWORD_TYPE, KEYWORD_TYPE_HASH)};
    const bool has_type{type != nullptr};
    const JSON::String *const type_string{
        type != nullptr && type->is_string() ? &type->to_string() : nullptr};
    const bool is_json{type_string != nullptr && *type_string == KEYWORD_JSON};
    // A JSON literal is a 1.1 feature, so a 1.0 processor rejects the value of
    // a value object the input types as @json, whatever that value is (JSON-LD
    // 1.1 API Section 5.1.2 step 13.4.7.1)
    if (is_json && state.processing_1_0) {
      throw JSONLDError("Invalid value object value",
                        origin_pointer(origins.value, pointer));
    }
    for (const auto &entry : result.as_object()) {
      if (!entry.key_equals(KEYWORD_VALUE, KEYWORD_VALUE_HASH) &&
          !entry.key_equals(KEYWORD_TYPE, KEYWORD_TYPE_HASH) &&
          !entry.key_equals(KEYWORD_LANGUAGE, KEYWORD_LANGUAGE_HASH) &&
          !entry.key_equals(KEYWORD_INDEX, KEYWORD_INDEX_HASH) &&
          !entry.key_equals(KEYWORD_DIRECTION, KEYWORD_DIRECTION_HASH)) {
        throw JSONLDError("Invalid value object",
                          offending_pointer(state, active_context, element,
                                            pointer, entry.first));
      }
      if ((entry.key_equals(KEYWORD_LANGUAGE, KEYWORD_LANGUAGE_HASH) ||
           entry.key_equals(KEYWORD_DIRECTION, KEYWORD_DIRECTION_HASH)) &&
          has_type) {
        throw JSONLDError("Invalid value object",
                          offending_pointer(state, active_context, element,
                                            pointer, entry.first));
      }
    }
    const auto &content{*value_entry};
    if (content.is_null() && !is_json) {
      return JSON{nullptr};
    }
    if (result.defines(KEYWORD_LANGUAGE, KEYWORD_LANGUAGE_HASH) &&
        !content.is_string()) {
      throw JSONLDError("Invalid language-tagged value",
                        origin_pointer(origins.value, pointer));
    }
    // The datatype of a typed value must be an IRI, which a blank node
    // identifier is not (JSON-LD 1.1 API Section 5.1.2 step 15.5)
    if (has_type && !is_json &&
        (type_string == nullptr || !URI::is_iri(*type_string))) {
      throw JSONLDError("Invalid typed value",
                        origin_pointer(origins.type, pointer));
    }
    if (!is_json && !content.is_string() && !content.is_number() &&
        !content.is_boolean()) {
      throw JSONLDError("Invalid value object value",
                        origin_pointer(origins.value, pointer));
    }
  } else if (const auto *type_entry{
                 result.try_at(KEYWORD_TYPE, KEYWORD_TYPE_HASH)};
             type_entry != nullptr && !type_entry->is_array()) {
    // Node objects always carry @type as an array.
    result.assign(KEYWORD_TYPE, into_array(JSON{*type_entry}));
  }

  // A set or list object may only carry an @index entry besides, and this is
  // validated before any value is dropped.
  if (result.defines(KEYWORD_LIST, KEYWORD_LIST_HASH) ||
      result.defines(KEYWORD_SET, KEYWORD_SET_HASH)) {
    // Neither collection keyword counts as the one other entry the other may
    // carry, so a map that defines both is invalid whatever else it holds
    // (JSON-LD 1.1 API Section 5.1.2 step 17.1).
    if (result.defines(KEYWORD_LIST, KEYWORD_LIST_HASH) &&
        result.defines(KEYWORD_SET, KEYWORD_SET_HASH)) {
      throw JSONLDError("Invalid set or list object",
                        origin_pointer(origins.collection, pointer));
    }

    for (const auto &entry : result.as_object()) {
      if (!entry.key_equals(KEYWORD_LIST, KEYWORD_LIST_HASH) &&
          !entry.key_equals(KEYWORD_SET, KEYWORD_SET_HASH) &&
          !entry.key_equals(KEYWORD_INDEX, KEYWORD_INDEX_HASH)) {
        throw JSONLDError("Invalid set or list object",
                          offending_pointer(state, active_context, element,
                                            pointer, entry.first,
                                            origins.collection));
      }
    }
  }

  // A bare @set collapses to its array.
  if (const auto *set{result.try_at(KEYWORD_SET, KEYWORD_SET_HASH)}) {
    return *set;
  }

  // Drop an incomplete value object that has a language or direction but no
  // value.
  if (!result.defines(KEYWORD_VALUE, KEYWORD_VALUE_HASH) &&
      (result.defines(KEYWORD_LANGUAGE, KEYWORD_LANGUAGE_HASH) ||
       result.defines(KEYWORD_DIRECTION, KEYWORD_DIRECTION_HASH))) {
    bool only_value_keys{true};
    for (const auto &entry : result.as_object()) {
      if (!entry.key_equals(KEYWORD_LANGUAGE, KEYWORD_LANGUAGE_HASH) &&
          !entry.key_equals(KEYWORD_DIRECTION, KEYWORD_DIRECTION_HASH) &&
          !entry.key_equals(KEYWORD_INDEX, KEYWORD_INDEX_HASH)) {
        only_value_keys = false;
      }
    }
    if (only_value_keys) {
      return JSON{nullptr};
    }
  }

  // Drop free-floating values when not under a property.
  if (!active_property.has_value() ||
      active_property.value() == KEYWORD_GRAPH) {
    if (result.empty() || result.defines(KEYWORD_VALUE, KEYWORD_VALUE_HASH) ||
        result.defines(KEYWORD_LIST, KEYWORD_LIST_HASH) ||
        (result.object_size() == 1 &&
         result.defines(KEYWORD_ID, KEYWORD_ID_HASH))) {
      return JSON{nullptr};
    }
  }

  return result;
}

auto expand_entries(ExpansionState &state, ActiveContext &active_context,
                    const ActiveContext &type_context, JSON &result,
                    KeywordOrigins &origins,
                    const std::optional<JSON::String> &active_property,
                    const JSON &source, const WeakPointer &source_pointer)
    -> void {
  // @nest entries are deferred and processed after the direct ones. The
  // property is referenced from the source object, never copied.
  // The index locates the nested object inside an array-valued entry, so that
  // what goes wrong within it names the element it came from
  struct NestEntry {
    const JSON::String *property{nullptr};
    const JSON *value{nullptr};
    std::optional<std::size_t> index{};
  };
  std::vector<NestEntry> nests;
  for (const auto &[key_pointer, value_pointer] : sorted_entries(source)) {
    const std::pair<const JSON::String &, const JSON &> entry{*key_pointer,
                                                              *value_pointer};
    const JSON::String &property{entry.first};
    const WeakPointer entry_pointer{source_pointer.concat(property)};
    if (property == KEYWORD_CONTEXT) {
      continue;
    }

    const auto expanded_property{expand_iri(state, active_context, property,
                                            false, true, nullptr, nullptr,
                                            EMPTY_WEAK_POINTER)};

    // Step 13.4.1: "If active property equals @reverse, an invalid reverse
    // property map error has been detected and processing is aborted". It is
    // the first substep of step 13.4, so it precedes every other keyword
    // branch, step 13.4.14's deferral of @nest included (JSON-LD 1.1 API
    // Section 5.1.2 step 13.4.1)
    if (expanded_property.has_value() &&
        is_keyword(expanded_property.value()) && active_property.has_value() &&
        active_property.value() == KEYWORD_REVERSE) {
      throw JSONLDError("Invalid reverse property map", entry_pointer);
    }

    if (expanded_property.has_value() &&
        expanded_property.value() == KEYWORD_NEST) {
      if (entry.second.is_array()) {
        std::size_t nest_index{0};
        for (const auto &nest_value : entry.second.as_array()) {
          nests.push_back({.property = &property,
                           .value = &nest_value,
                           .index = nest_index});
          nest_index += 1;
        }
      } else {
        nests.push_back(
            {.property = &property, .value = &entry.second, .index = {}});
      }
      continue;
    }
    if (!expanded_property.has_value()) {
      continue;
    }

    const auto &name{expanded_property.value()};
    if (!name.contains(':') && !is_keyword(name)) {
      continue;
    }

    // The @type and @included exemption from colliding keywords does not apply
    // in json-ld-1.0.
    if (is_keyword(name) && result.defines(name) &&
        (state.processing_1_0 ||
         (name != KEYWORD_TYPE && name != KEYWORD_INCLUDED))) {
      throw JSONLDError("Colliding keywords", entry_pointer);
    }

    if (name == KEYWORD_ID) {
      if (!entry.second.is_string()) {
        throw JSONLDError("Invalid @id value", entry_pointer);
      }
      const auto identifier{expand_iri(state, active_context,
                                       entry.second.to_string(), true, false,
                                       nullptr, nullptr, EMPTY_WEAK_POINTER)};
      if (identifier.has_value()) {
        result.assign_assume_new(JSON::String{KEYWORD_ID},
                                 JSON{identifier.value()}, KEYWORD_ID_HASH);
      } else {
        result.assign_assume_new(JSON::String{KEYWORD_ID}, JSON{nullptr},
                                 KEYWORD_ID_HASH);
      }
      continue;
    }

    if (name == KEYWORD_TYPE) {
      if (entry.second.is_array()) {
        std::size_t type_index{0};
        for (const auto &item : entry.second.as_array()) {
          if (!item.is_string()) {
            throw JSONLDError("Invalid type value",
                              entry_pointer.concat(type_index));
          }
          type_index += 1;
        }
      } else if (!entry.second.is_string()) {
        throw JSONLDError("Invalid type value", entry_pointer);
      }
      // Expand each value, preserving whether the input was a string or an
      // array. The node-object post-processing later turns a lone string into
      // an array, but a value object keeps its @type as a string.
      JSON expanded_type{nullptr};
      if (entry.second.is_array()) {
        expanded_type = JSON::make_array();
        for (const auto &item : entry.second.as_array()) {
          // A value that does not expand to an IRI is omitted, so @type stays
          // an array of strings.
          auto type{expand_type(state, type_context, item)};
          if (!type.is_null()) {
            expanded_type.push_back(std::move(type));
          }
        }
      } else {
        expanded_type = expand_type(state, type_context, entry.second);
      }
      // A lone @type value that does not expand to an IRI carries nothing,
      // which includes carrying no blame for what the element ends up with
      if (expanded_type.is_null()) {
        continue;
      }
      origins.type = entry_pointer;
      if (result.defines(KEYWORD_TYPE, KEYWORD_TYPE_HASH)) {
        auto merged{
            into_array(std::move(result.at(KEYWORD_TYPE, KEYWORD_TYPE_HASH)))};
        auto expanded_type_array{into_array(std::move(expanded_type))};
        for (auto &item : expanded_type_array.as_array()) {
          merged.push_back(item);
        }
        result.assign(KEYWORD_TYPE, std::move(merged));
      } else {
        result.assign_assume_new(JSON::String{KEYWORD_TYPE},
                                 std::move(expanded_type), KEYWORD_TYPE_HASH);
      }
      continue;
    }

    if (name == KEYWORD_VALUE) {
      origins.value = entry_pointer;
      result.assign_assume_new(JSON::String{KEYWORD_VALUE}, JSON{entry.second},
                               KEYWORD_VALUE_HASH);
      continue;
    }

    if (name == KEYWORD_LANGUAGE) {
      if (!entry.second.is_string()) {
        throw JSONLDError("Invalid language-tagged string", entry_pointer);
      }
      result.assign_assume_new(JSON::String{KEYWORD_LANGUAGE},
                               JSON{entry.second}, KEYWORD_LANGUAGE_HASH);
      continue;
    }

    if (name == KEYWORD_DIRECTION) {
      if (state.processing_1_0) {
        continue;
      }
      if (!entry.second.is_string() || (entry.second.to_string() != "ltr" &&
                                        entry.second.to_string() != "rtl")) {
        throw JSONLDError("Invalid base direction", entry_pointer);
      }
      result.assign_assume_new(JSON::String{KEYWORD_DIRECTION},
                               JSON{entry.second}, KEYWORD_DIRECTION_HASH);
      continue;
    }

    if (name == KEYWORD_LIST || name == KEYWORD_SET) {
      // A free-floating list is removed before its items are expanded
      // (JSON-LD 1.1 API Section 5.1.2 step 13.4.11.1)
      if (name == KEYWORD_LIST && (!active_property.has_value() ||
                                   active_property.value() == KEYWORD_GRAPH)) {
        continue;
      }

      // A set object expands its value as it stands rather than as an array,
      // so a null value expands to null and the collapse below leaves nothing
      // for the enclosing property to carry (JSON-LD 1.1 API Section 5.1.2
      // steps 13.4.12 and 18.2)
      if (name == KEYWORD_SET && entry.second.is_null()) {
        origins.collection = source_pointer;
        result.assign(name, JSON{nullptr});
        continue;
      }

      origins.collection = source_pointer;
      auto elements{JSON::make_array()};
      const auto values{into_array(JSON{entry.second})};
      std::size_t value_index{0};
      for (const auto &item : values.as_array()) {
        const WeakPointer item_pointer{entry.second.is_array()
                                           ? entry_pointer.concat(value_index)
                                           : entry_pointer};
        auto expanded_item{
            expand(state, active_context, active_property, item, item_pointer)};
        if (expanded_item.is_array()) {
          for (auto &nested : expanded_item.as_array()) {
            elements.push_back(nested);
          }
        } else if (!expanded_item.is_null()) {
          elements.push_back(std::move(expanded_item));
        }
        value_index += 1;
      }
      if (name == KEYWORD_LIST && state.processing_1_0) {
        for (const auto &item : elements.as_array()) {
          if (is_list_object(item)) {
            throw JSONLDError("List of lists",
                              forbidden_member_pointer(
                                  state, active_context, active_property,
                                  entry.second, entry_pointer, is_list_object));
          }
        }
      }
      result.assign(name, std::move(elements));
      continue;
    }

    if (name == KEYWORD_GRAPH) {
      // @graph expands to an array of node objects, so a value that expands to
      // null contributes no element rather than a null one.
      auto graph{expand(state, active_context, JSON::String{KEYWORD_GRAPH},
                        entry.second, entry_pointer)};
      merge(result, KEYWORD_GRAPH,
            graph.is_null() ? JSON::make_array()
                            : into_array(std::move(graph)));
      continue;
    }

    if (name == KEYWORD_INCLUDED) {
      if (state.processing_1_0) {
        continue;
      }
      // Step 13.4.7.2 expands the value with a null active property, which
      // would drop the free-floating values step 13.4.7.3 has to reject, so
      // the official suite requires a value that is a scalar, a value object,
      // or a list object to be an error rather than nothing (suite entries
      // #tin07 to #tin09). The keyword stands in as the active property so
      // that nothing is dropped before being judged, which also keeps the
      // identifier-only node references that the step admits. Each member is
      // expanded on its own, so what goes wrong names the input position it
      // came from (JSON-LD 1.1 API Section 5.1.2 steps 13.4.7.2 and 13.4.7.3)
      auto included{JSON::make_array()};
      const bool from_array{entry.second.is_array()};
      const auto members{into_array(JSON{entry.second})};
      std::size_t member_index{0};
      for (const auto &member : members.as_array()) {
        const WeakPointer member_pointer{
            from_array ? entry_pointer.concat(member_index) : entry_pointer};
        auto expanded_member{expand(state, active_context,
                                    JSON::String{KEYWORD_INCLUDED}, member,
                                    member_pointer)};
        // A null member of an array carries nothing over, the way step 5.2.3
        // drops one from any array, whereas a lone null value is no included
        // block at all and is held to the same account as any other member
        // (JSON-LD 1.1 Section 9.13, JSON-LD 1.1 API Section 5.1.2 step 5.2.3)
        auto member_items{from_array && expanded_member.is_null()
                              ? JSON::make_array()
                              : into_array(std::move(expanded_member))};
        for (auto &item : member_items.as_array()) {
          if (is_not_node_object(item)) {
            throw JSONLDError(
                "Invalid @included value",
                forbidden_member_pointer(state, active_context,
                                         JSON::String{KEYWORD_INCLUDED}, member,
                                         member_pointer, is_not_node_object));
          }
          included.push_back(std::move(item));
        }
        member_index += 1;
      }
      merge(result, KEYWORD_INCLUDED, std::move(included));
      continue;
    }

    if (name == KEYWORD_INDEX) {
      if (!entry.second.is_string()) {
        throw JSONLDError("Invalid @index value", entry_pointer);
      }
      result.assign_assume_new(JSON::String{KEYWORD_INDEX}, JSON{entry.second},
                               KEYWORD_INDEX_HASH);
      continue;
    }

    if (name == KEYWORD_REVERSE) {
      if (!entry.second.is_object()) {
        throw JSONLDError("Invalid @reverse value", entry_pointer);
      }
      auto reversed{expand(state, active_context, JSON::String{KEYWORD_REVERSE},
                           entry.second, entry_pointer)};
      if (reversed.is_object()) {
        const auto *existing_reverse{
            result.try_at(KEYWORD_REVERSE, KEYWORD_REVERSE_HASH)};
        auto reverse_map{existing_reverse != nullptr ? *existing_reverse
                                                     : JSON::make_object()};
        for (const auto &reverse_entry : reversed.as_object()) {
          const auto &reverse_property{reverse_entry.first};
          if (reverse_entry.key_equals(KEYWORD_REVERSE, KEYWORD_REVERSE_HASH)) {
            for (const auto &forward : reverse_entry.second.as_object()) {
              merge(result, JSON::StringView{forward.first},
                    into_array(JSON{forward.second}));
            }
          } else {
            const auto reverse_values{into_array(JSON{reverse_entry.second})};
            for (const auto &item : reverse_values.as_array()) {
              if (is_value_or_list_object(item)) {
                // The map carries its own context into how its keys expand, so
                // what it says wrong is located against that same context,
                // which is only worth putting together to report it. Reading
                // it again outside the expansion that already took it may not
                // go through, and a location is not worth an error of its own
                ActiveContext reverse_context{active_context};
                if (entry.second.defines(KEYWORD_CONTEXT,
                                         KEYWORD_CONTEXT_HASH)) {
                  try {
                    process_context(
                        state, reverse_context,
                        entry.second.at(KEYWORD_CONTEXT, KEYWORD_CONTEXT_HASH),
                        entry_pointer.concat(keyword_context()));
                  } catch (const JSONLDError &) {
                    reverse_context = active_context;
                  }
                }

                // Several keys of the map may reach the same reverse
                // property, and what they carry is merged before it is
                // inspected, so each is expanded again to find the one that
                // holds the offending member
                for (const auto &[input_key, input_value] :
                     sorted_entries(entry.second)) {
                  const auto input_property{
                      expand_iri(state, reverse_context, *input_key, false,
                                 true, nullptr, nullptr, EMPTY_WEAK_POINTER)};
                  if (!input_property.has_value() ||
                      input_property.value() != reverse_property) {
                    continue;
                  }
                  const WeakPointer input_pointer{
                      entry_pointer.concat(*input_key)};
                  if (holds_forbidden_member(expand(state, reverse_context,
                                                    *input_key, *input_value,
                                                    input_pointer),
                                             is_value_or_list_object)) {
                    throw JSONLDError("Invalid reverse property value",
                                      forbidden_member_pointer(
                                          state, reverse_context, *input_key,
                                          *input_value, input_pointer,
                                          is_value_or_list_object));
                  }
                }

                throw JSONLDError("Invalid reverse property value",
                                  entry_pointer);
              }
            }
            merge(reverse_map, reverse_property,
                  into_array(JSON{reverse_entry.second}));
          }
        }
        result.assign(KEYWORD_REVERSE, std::move(reverse_map));
      }
      continue;
    }

    if (is_keyword(name)) {
      // Keywords with no property-level handler (such as @vocab or @none
      // reached through an alias) carry no expanded value, so the Expansion
      // algorithm adds nothing to the result for them.
      continue;
    }

    const TermDefinition *definition{nullptr};
    const auto term{active_context.terms.find(property)};
    if (term != active_context.terms.cend()) {
      definition = &term->second;
    }

    JSON expanded_value{nullptr};
    if (definition != nullptr && definition->type_mapping.has_value() &&
        definition->type_mapping.value() == KEYWORD_JSON) {
      // A term coerced to @json keeps its value verbatim.
      auto json_value{JSON::make_object()};
      json_value.assign_assume_new(JSON::String{KEYWORD_VALUE},
                                   JSON{entry.second}, KEYWORD_VALUE_HASH);
      json_value.assign_assume_new(JSON::String{KEYWORD_TYPE},
                                   JSON{KEYWORD_JSON}, KEYWORD_TYPE_HASH);
      expanded_value = std::move(json_value);
    } else if (entry.second.is_object() &&
               container_includes(definition, KEYWORD_GRAPH) &&
               (container_includes(definition, KEYWORD_ID) ||
                container_includes(definition, KEYWORD_INDEX))) {
      const bool by_id{container_includes(definition, KEYWORD_ID)};
      const bool property_valued{definition->index.has_value() &&
                                 definition->index.value() != KEYWORD_INDEX};
      std::optional<JSON::String> index_property;
      if (property_valued) {
        index_property =
            expand_iri(state, active_context, definition->index.value(), false,
                       true, nullptr, nullptr, EMPTY_WEAK_POINTER);
      }
      expanded_value = JSON::make_array();
      for (const auto &[graph_key, graph_value] :
           sorted_entries(entry.second)) {
        const JSON::String &index{*graph_key};
        const auto expanded_key{index == KEYWORD_NONE
                                    ? std::optional<JSON::String>{KEYWORD_NONE}
                                    : expand_iri(state, active_context, index,
                                                 true, false, nullptr, nullptr,
                                                 EMPTY_WEAK_POINTER)};
        const bool none_key{expanded_key.has_value() &&
                            expanded_key.value() == KEYWORD_NONE};
        auto graph_items{
            into_array(expand(state, active_context, property, *graph_value,
                              entry_pointer.concat(index), true))};
        for (auto &item : graph_items.as_array()) {
          // Nothing is carried over for a null, as in an array (JSON-LD 1.1
          // API Section 5.1.2 step 5.2.3)
          if (item.is_null()) {
            continue;
          }

          // Wrap the item in a graph object, unless it is already one.
          JSON graph{nullptr};
          if (item.is_object() &&
              item.defines(KEYWORD_GRAPH, KEYWORD_GRAPH_HASH)) {
            graph = std::move(item);
          } else {
            graph = JSON::make_object();
            graph.assign_assume_new(JSON::String{KEYWORD_GRAPH},
                                    into_array(std::move(item)),
                                    KEYWORD_GRAPH_HASH);
          }
          if (!none_key) {
            if (by_id) {
              if (!graph.defines(KEYWORD_ID, KEYWORD_ID_HASH)) {
                graph.assign_assume_new(JSON::String{KEYWORD_ID},
                                        JSON{expanded_key.value_or(index)},
                                        KEYWORD_ID_HASH);
              }
            } else if (property_valued) {
              // A term that no longer names anything carries no metadata
              // (JSON-LD 1.1 API Section 5.1.2 step 13.8.3.7.2)
              if (index_property.has_value()) {
                auto combined{into_array(expand_value(
                    state, active_context, definition->index, JSON{index}))};
                if (graph.defines(index_property.value())) {
                  for (auto &existing :
                       graph.at(index_property.value()).as_array()) {
                    combined.push_back(existing);
                  }
                }
                graph.assign(index_property.value(), std::move(combined));
              }
            } else if (!graph.defines(KEYWORD_INDEX, KEYWORD_INDEX_HASH)) {
              graph.assign_assume_new(JSON::String{KEYWORD_INDEX}, JSON{index},
                                      KEYWORD_INDEX_HASH);
            }
          }
          expanded_value.push_back(std::move(graph));
        }
      }
    } else if (entry.second.is_object() &&
               container_includes(definition, KEYWORD_LANGUAGE)) {
      expanded_value = JSON::make_array();
      for (const auto &[language_key, language_value] :
           sorted_entries(entry.second)) {
        const JSON::String &language{*language_key};
        const auto expanded_language{expand_iri(state, active_context, language,
                                                false, true, nullptr, nullptr,
                                                EMPTY_WEAK_POINTER)};
        const bool is_none{language == KEYWORD_NONE ||
                           (expanded_language.has_value() &&
                            expanded_language.value() == KEYWORD_NONE)};
        const WeakPointer language_pointer{entry_pointer.concat(language)};
        const bool language_array{language_value->is_array()};
        auto language_items{into_array(JSON{*language_value})};
        std::size_t language_index{0};
        for (auto &item : language_items.as_array()) {
          const WeakPointer item_pointer{
              language_array ? language_pointer.concat(language_index)
                             : language_pointer};
          language_index += 1;
          if (item.is_null()) {
            continue;
          }
          if (!item.is_string()) {
            throw JSONLDError("Invalid language map value", item_pointer);
          }
          auto value{JSON::make_object()};
          value.assign_assume_new(JSON::String{KEYWORD_VALUE}, JSON{item},
                                  KEYWORD_VALUE_HASH);
          if (!is_none) {
            value.assign_assume_new(JSON::String{KEYWORD_LANGUAGE},
                                    JSON{language}, KEYWORD_LANGUAGE_HASH);
          }
          const auto direction{definition->has_direction
                                   ? definition->direction
                                   : active_context.default_direction};
          if (direction.has_value()) {
            value.assign_assume_new(JSON::String{KEYWORD_DIRECTION},
                                    JSON{direction.value()},
                                    KEYWORD_DIRECTION_HASH);
          }
          expanded_value.push_back(std::move(value));
        }
      }
    } else if (entry.second.is_object() &&
               container_includes(definition, KEYWORD_INDEX)) {
      const bool property_valued{definition->index.has_value() &&
                                 definition->index.value() != KEYWORD_INDEX};
      std::optional<JSON::String> index_property;
      if (property_valued) {
        index_property =
            expand_iri(state, active_context, definition->index.value(), false,
                       true, nullptr, nullptr, EMPTY_WEAK_POINTER);
      }
      expanded_value = JSON::make_array();
      for (const auto &[index_key, index_value] :
           sorted_entries(entry.second)) {
        const JSON::String &index{*index_key};
        // Step 13.8.3.4 initialises "expanded index to the result of IRI
        // expanding index", and every step that goes on to add index metadata
        // requires that "expanded index is not @none", so an alias of the
        // keyword suppresses the metadata exactly as the keyword does
        // (JSON-LD 1.1 API Section 5.1.2 steps 13.8.3.4 and 13.8.3.7.2 to
        // 13.8.3.7.5)
        const auto expanded_index{expand_iri(state, active_context, index,
                                             false, true, nullptr, nullptr,
                                             EMPTY_WEAK_POINTER)};
        const bool index_is_none{expanded_index.has_value() &&
                                 expanded_index.value() == KEYWORD_NONE};
        auto index_items{
            into_array(expand(state, active_context, property, *index_value,
                              entry_pointer.concat(index), true))};
        for (auto &item : index_items.as_array()) {
          // An entry of a map expands the way it would as a member of an
          // array, where nothing is carried over for a null (JSON-LD 1.1 API
          // Section 5.1.2 step 5.2.3), so the steps below only ever see a value
          if (item.is_null()) {
            continue;
          }

          if (!index_is_none) {
            if (property_valued) {
              // A term that no longer names anything carries no metadata, and
              // the value object check that follows the addition does not
              // apply when nothing was added (JSON-LD 1.1 API Section 5.1.2
              // step 13.8.3.7.2)
              if (index_property.has_value()) {
                if (is_value_object(item)) {
                  throw JSONLDError(
                      "Invalid value object",
                      forbidden_member_pointer(
                          state, active_context, property, *index_value,
                          entry_pointer.concat(index), is_value_object));
                }
                // The index value is prepended to any existing values.
                auto combined{into_array(expand_value(
                    state, active_context, definition->index, JSON{index}))};
                if (item.defines(index_property.value())) {
                  for (auto &existing :
                       item.at(index_property.value()).as_array()) {
                    combined.push_back(existing);
                  }
                }
                item.assign(index_property.value(), std::move(combined));
              }
            } else if (!item.defines(KEYWORD_INDEX, KEYWORD_INDEX_HASH)) {
              item.assign_assume_new(JSON::String{KEYWORD_INDEX}, JSON{index},
                                     KEYWORD_INDEX_HASH);
            }
          }
          expanded_value.push_back(item);
        }
      }
    } else if (entry.second.is_object() &&
               (container_includes(definition, KEYWORD_ID) ||
                container_includes(definition, KEYWORD_TYPE))) {
      const bool by_id{container_includes(definition, KEYWORD_ID)};
      expanded_value = JSON::make_array();
      for (const auto &[map_key, map_value] : sorted_entries(entry.second)) {
        const JSON::String &index{*map_key};
        std::optional<JSON::String> expanded_index;
        if (index != KEYWORD_NONE) {
          expanded_index =
              expand_iri(state, active_context, index, by_id, !by_id, nullptr,
                         nullptr, EMPTY_WEAK_POINTER);
        }
        // The key may be an alias of @none, which carries no identifier.
        if (expanded_index.has_value() &&
            expanded_index.value() == KEYWORD_NONE) {
          expanded_index = std::nullopt;
        }
        // A type map key may carry a type-scoped context for its values.
        // Type-scoped contexts do not propagate, so the values are resolved
        // against the context that preceded the containing type-scoped
        // context, with only this key's context layered on top.
        const ActiveContext &base_context{active_context.previous && !by_id
                                              ? *active_context.previous
                                              : active_context};
        ActiveContext entry_context{base_context};
        if (!by_id) {
          // Resolve the type term against the context the copy was made from,
          // which outlives the copy being mutated below.
          const auto type_definition{base_context.terms.find(index)};
          if (type_definition != base_context.terms.cend() &&
              type_definition->second.context.has_value()) {
            process_deferred_scoped_context(state, entry_context,
                                            type_definition->second, true);
            entry_context.previous = nullptr;
          }
        }
        // String values in a type map are node references. The recursion the
        // other values take would expand them under the property's own
        // context, so the shortcut carries it too (JSON-LD 1.1 API Section
        // 5.1.2 steps 13.8.3.6 and 4.2)
        ActiveContext reference_scope;
        ActiveContext *reference_context{&entry_context};
        if (!by_id && definition->context.has_value()) {
          reference_scope = entry_context;
          reference_scope.previous = nullptr;
          apply_scoped_context(state, reference_scope, *definition);
          reference_context = &reference_scope;
        }

        auto entries{JSON::make_array()};
        const bool value_array{map_value->is_array()};
        auto raw_values{into_array(JSON{*map_value})};
        std::size_t raw_index{0};
        for (auto &raw : raw_values.as_array()) {
          // An entry holding an array is located by the element the value came
          // from, as an array of properties is
          const WeakPointer entry_value_pointer{entry_pointer.concat(index)};
          const WeakPointer raw_pointer{
              value_array ? entry_value_pointer.concat(raw_index)
                          : entry_value_pointer};
          raw_index += 1;
          if (raw.is_string() && !by_id) {
            auto reference{JSON::make_object()};
            const bool reference_vocab{definition->type_mapping.has_value() &&
                                       definition->type_mapping.value() ==
                                           KEYWORD_VOCAB};
            const auto &raw_string{raw.to_string()};
            const auto referenced{expand_iri(
                state, *reference_context, raw_string, true, reference_vocab,
                nullptr, nullptr, EMPTY_WEAK_POINTER)};
            reference.assign_assume_new(JSON::String{KEYWORD_ID},
                                        JSON{referenced.value_or(raw_string)},
                                        KEYWORD_ID_HASH);
            entries.push_back(std::move(reference));
          } else {
            auto expanded_items{into_array(expand(
                state, entry_context, property, raw, raw_pointer, true))};
            for (auto &expanded : expanded_items.as_array()) {
              // Nothing is carried over for a null, as in an array
              // (JSON-LD 1.1 API Section 5.1.2 step 5.2.3)
              if (!expanded.is_null()) {
                entries.push_back(expanded);
              }
            }
          }
        }
        for (auto &item : entries.as_array()) {
          if (expanded_index.has_value()) {
            if (by_id) {
              if (!item.defines(KEYWORD_ID, KEYWORD_ID_HASH)) {
                item.assign_assume_new(JSON::String{KEYWORD_ID},
                                       JSON{expanded_index.value()},
                                       KEYWORD_ID_HASH);
              }
            } else {
              auto types{JSON::make_array()};
              types.push_back(JSON{expanded_index.value()});
              if (item.defines(KEYWORD_TYPE, KEYWORD_TYPE_HASH)) {
                auto existing_types{into_array(
                    std::move(item.at(KEYWORD_TYPE, KEYWORD_TYPE_HASH)))};
                for (auto &existing : existing_types.as_array()) {
                  types.push_back(existing);
                }
              }
              item.assign(KEYWORD_TYPE, std::move(types));
            }
          }
          expanded_value.push_back(item);
        }
      }
    } else if (container_includes(definition, KEYWORD_GRAPH)) {
      auto graph_value{
          expand(state, active_context, property, entry.second, entry_pointer)};
      // Nothing is carried over for a null, which is dropped before a graph
      // object could wrap it (JSON-LD 1.1 API Section 5.1.2 steps 13.10 and
      // 13.12)
      if (graph_value.is_null()) {
        continue;
      }

      expanded_value = JSON::make_array();
      auto graph_items{into_array(std::move(graph_value))};
      for (auto &item : graph_items.as_array()) {
        auto graph{JSON::make_object()};
        graph.assign_assume_new(JSON::String{KEYWORD_GRAPH},
                                into_array(std::move(item)),
                                KEYWORD_GRAPH_HASH);
        expanded_value.push_back(std::move(graph));
      }
    } else {
      expanded_value =
          expand(state, active_context, property, entry.second, entry_pointer);
    }

    // A @list container wraps the expanded value, including a @json-coerced
    // one.
    if (container_includes(definition, KEYWORD_LIST) &&
        !expanded_value.is_null() &&
        !(expanded_value.is_object() &&
          expanded_value.defines(KEYWORD_LIST, KEYWORD_LIST_HASH))) {
      expanded_value = into_array(std::move(expanded_value));
      if (state.processing_1_0) {
        for (const auto &item : expanded_value.as_array()) {
          if (is_list_object(item)) {
            throw JSONLDError("List of lists",
                              forbidden_member_pointer(
                                  state, active_context, property, entry.second,
                                  entry_pointer, is_list_object));
          }
        }
      }
      auto wrapper{JSON::make_object()};
      wrapper.assign_assume_new(JSON::String{KEYWORD_LIST},
                                std::move(expanded_value), KEYWORD_LIST_HASH);
      expanded_value = std::move(wrapper);
    }

    if (expanded_value.is_null()) {
      continue;
    }

    if (definition != nullptr && definition->reverse) {
      const auto reverse_items{into_array(JSON{expanded_value})};
      for (const auto &item : reverse_items.as_array()) {
        if (is_value_or_list_object(item)) {
          throw JSONLDError("Invalid reverse property value",
                            forbidden_member_pointer(
                                state, active_context, property, entry.second,
                                entry_pointer, is_value_or_list_object));
        }
      }
      const auto *existing_reverse{
          result.try_at(KEYWORD_REVERSE, KEYWORD_REVERSE_HASH)};
      auto reverse_map{existing_reverse != nullptr ? *existing_reverse
                                                   : JSON::make_object()};
      merge(reverse_map, name, into_array(std::move(expanded_value)));
      result.assign(KEYWORD_REVERSE, std::move(reverse_map));
      continue;
    }

    merge(result, name, into_array(std::move(expanded_value)));
  }
  for (const auto &nest_entry : nests) {
    // A @nest alias term may carry a property-scoped context for the nested
    // entries. That context lives in the term definition rather than under the
    // element, so it keeps the alias key as its location while the element
    // itself is located by index where the entry held an array
    const WeakPointer nest_pointer{source_pointer.concat(*nest_entry.property)};
    const WeakPointer element_pointer{
        nest_entry.index.has_value()
            ? nest_pointer.concat(nest_entry.index.value())
            : nest_pointer};
    const auto definition{active_context.terms.find(*nest_entry.property)};
    const bool scoped{definition != active_context.terms.cend() &&
                      definition->second.context.has_value()};
    ActiveContext nested;
    if (scoped) {
      // Process the scoped context into a copy so the term that owns it is not
      // freed while it is being read.
      nested = active_context;
      apply_scoped_context(state, nested, definition->second);
      nested.previous = nullptr;
    }

    ActiveContext &nest_context{scoped ? nested : active_context};
    // The nested entries expand under the context the term brings, so a value
    // entry that only that context names is forbidden just the same (JSON-LD
    // 1.1 API Section 5.1.2 step 14.2.1)
    if (!nest_entry.value->is_object() ||
        nest_defines_value(state, nest_context, *nest_entry.value)) {
      throw JSONLDError("Invalid @nest value", element_pointer);
    }

    expand_entries(state, nest_context, type_context, result, origins,
                   active_property, *nest_entry.value, element_pointer);
  }
}

} // namespace

// Expansion (JSON-LD 1.1 API Section 5.1.2)
auto expand(ExpansionState &state, ActiveContext &active_context,
            const std::optional<JSON::String> &active_property,
            const JSON &element, const WeakPointer &pointer,
            const bool from_map) -> JSON {
  const NestingDepthScope scope{state.depth};
  if (state.depth > ExpansionState::MAXIMUM_DEPTH) {
    throw JSONLDError("Maximum nesting depth exceeded", pointer);
  }

  if (element.is_null()) {
    return JSON{nullptr};
  }

  // The active property may carry a context of its own, which covers the
  // values it is expanded from (JSON-LD 1.1 API Section 5.1.2 step 3)
  const TermDefinition *scoped_definition{nullptr};
  if (active_property.has_value()) {
    const auto scoped_term{active_context.terms.find(active_property.value())};
    if (scoped_term != active_context.terms.cend() &&
        scoped_term->second.context.has_value()) {
      scoped_definition = &scoped_term->second;
    }
  }

  if (!element.is_object() && !element.is_array()) {
    if (!active_property.has_value() ||
        active_property.value() == KEYWORD_GRAPH) {
      return JSON{nullptr};
    }

    // (JSON-LD 1.1 API Section 5.1.2 step 4.2)
    if (scoped_definition != nullptr) {
      ActiveContext scoped{active_context};
      scoped.previous = nullptr;
      apply_scoped_context(state, scoped, *scoped_definition);
      return expand_value(state, scoped, active_property, element);
    }

    return expand_value(state, active_context, active_property, element);
  }

  if (element.is_array()) {
    const TermDefinition *definition{nullptr};
    if (active_property.has_value()) {
      const auto term{active_context.terms.find(active_property.value())};
      if (term != active_context.terms.cend()) {
        definition = &term->second;
      }
    }

    auto result{JSON::make_array()};
    std::size_t item_index{0};
    for (const auto &item : element.as_array()) {
      // Each item stands where the array did, so a value taken from a
      // container map stays one (JSON-LD 1.1 API Section 5.1.2 step 5.2.1)
      auto expanded{expand(state, active_context, active_property, item,
                           pointer.concat(item_index), from_map)};
      if (expanded.is_array()) {
        for (auto &nested : expanded.as_array()) {
          result.push_back(nested);
        }
      } else if (!expanded.is_null()) {
        result.push_back(std::move(expanded));
      }
      item_index += 1;
    }

    if (container_includes(definition, KEYWORD_LIST)) {
      if (state.processing_1_0) {
        for (const auto &item : result.as_array()) {
          if (is_list_object(item)) {
            throw JSONLDError(
                "List of lists",
                forbidden_member_pointer(state, active_context, active_property,
                                         element, pointer, is_list_object));
          }
        }
      }
      auto wrapper{JSON::make_object()};
      wrapper.assign_assume_new(JSON::String{KEYWORD_LIST}, std::move(result),
                                KEYWORD_LIST_HASH);
      return wrapper;
    }

    return result;
  }

  // Revert a non-propagating (type-scoped) context when descending into a node
  // object that is neither a value object nor an @id-only reference. A value
  // the enclosing entry took from a container map is not such a descent
  // (JSON-LD 1.1 API Section 5.1.2 step 7).
  ActiveContext reverted;
  ActiveContext *current{&active_context};
  if (active_context.previous && !from_map) {
    bool value_or_id{false};
    const bool single{element.object_size() == 1};
    for (const auto &entry : element.as_object()) {
      const auto expanded{expand_iri(state, active_context, entry.first, false,
                                     true, nullptr, nullptr,
                                     EMPTY_WEAK_POINTER)};
      if (expanded.has_value() &&
          (expanded.value() == KEYWORD_VALUE ||
           (single && expanded.value() == KEYWORD_ID))) {
        value_or_id = true;
        break;
      }
    }
    if (!value_or_id) {
      reverted = *active_context.previous;
      current = &reverted;
    }
  }

  // The property-scoped context is layered on after the revert, so a
  // non-propagating one covers this node while the nodes inside it regain the
  // context the revert restored (JSON-LD 1.1 API Section 5.1.2 step 8)
  ActiveContext scoped;
  if (scoped_definition != nullptr) {
    scoped = *current;
    // A property-scoped context propagates by default, so it does not inherit
    // an enclosing type-scoped revert. It may, however, set its own revert
    // when it specifies @propagate: false.
    scoped.previous = nullptr;
    apply_scoped_context(state, scoped, *scoped_definition);
    current = &scoped;
  }

  if (element.defines(KEYWORD_CONTEXT, KEYWORD_CONTEXT_HASH)) {
    ActiveContext local{*current};
    process_context(state, local,
                    element.at(KEYWORD_CONTEXT, KEYWORD_CONTEXT_HASH),
                    pointer.concat(keyword_context()));
    return expand_object(state, local, active_property, element, pointer);
  }

  return expand_object(state, *current, active_property, element, pointer);
}

} // namespace sourcemeta::core
