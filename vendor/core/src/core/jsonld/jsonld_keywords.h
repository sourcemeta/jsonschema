#ifndef SOURCEMETA_CORE_JSONLD_KEYWORDS_H_
#define SOURCEMETA_CORE_JSONLD_KEYWORDS_H_

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/text.h>

#include <cstddef> // std::size_t

namespace sourcemeta::core {

// The JSON-LD 1.1 keywords (https://www.w3.org/TR/json-ld11/#keywords).
inline constexpr JSON::StringView KEYWORD_BASE{"@base"};
inline constexpr JSON::StringView KEYWORD_CONTAINER{"@container"};
inline constexpr JSON::StringView KEYWORD_CONTEXT{"@context"};
inline constexpr JSON::StringView KEYWORD_DIRECTION{"@direction"};
inline constexpr JSON::StringView KEYWORD_GRAPH{"@graph"};
inline constexpr JSON::StringView KEYWORD_ID{"@id"};
inline constexpr JSON::StringView KEYWORD_IMPORT{"@import"};
inline constexpr JSON::StringView KEYWORD_INCLUDED{"@included"};
inline constexpr JSON::StringView KEYWORD_INDEX{"@index"};
inline constexpr JSON::StringView KEYWORD_JSON{"@json"};
inline constexpr JSON::StringView KEYWORD_LANGUAGE{"@language"};
inline constexpr JSON::StringView KEYWORD_LIST{"@list"};
inline constexpr JSON::StringView KEYWORD_NEST{"@nest"};
inline constexpr JSON::StringView KEYWORD_NONE{"@none"};
inline constexpr JSON::StringView KEYWORD_PREFIX{"@prefix"};
inline constexpr JSON::StringView KEYWORD_PROPAGATE{"@propagate"};
inline constexpr JSON::StringView KEYWORD_PROTECTED{"@protected"};
inline constexpr JSON::StringView KEYWORD_REVERSE{"@reverse"};
inline constexpr JSON::StringView KEYWORD_SET{"@set"};
inline constexpr JSON::StringView KEYWORD_TYPE{"@type"};
inline constexpr JSON::StringView KEYWORD_VALUE{"@value"};
inline constexpr JSON::StringView KEYWORD_VERSION{"@version"};
inline constexpr JSON::StringView KEYWORD_VOCAB{"@vocab"};

// Precomputed object-key hashes for each keyword, so that object access never
// has to recompute them.
inline constexpr auto KEYWORD_BASE_HASH{JSON::Object::hash(KEYWORD_BASE)};
inline constexpr auto KEYWORD_CONTAINER_HASH{
    JSON::Object::hash(KEYWORD_CONTAINER)};
inline constexpr auto KEYWORD_CONTEXT_HASH{JSON::Object::hash(KEYWORD_CONTEXT)};
inline constexpr auto KEYWORD_DIRECTION_HASH{
    JSON::Object::hash(KEYWORD_DIRECTION)};
inline constexpr auto KEYWORD_GRAPH_HASH{JSON::Object::hash(KEYWORD_GRAPH)};
inline constexpr auto KEYWORD_ID_HASH{JSON::Object::hash(KEYWORD_ID)};
inline constexpr auto KEYWORD_IMPORT_HASH{JSON::Object::hash(KEYWORD_IMPORT)};
inline constexpr auto KEYWORD_INCLUDED_HASH{
    JSON::Object::hash(KEYWORD_INCLUDED)};
inline constexpr auto KEYWORD_INDEX_HASH{JSON::Object::hash(KEYWORD_INDEX)};
inline constexpr auto KEYWORD_JSON_HASH{JSON::Object::hash(KEYWORD_JSON)};
inline constexpr auto KEYWORD_LANGUAGE_HASH{
    JSON::Object::hash(KEYWORD_LANGUAGE)};
inline constexpr auto KEYWORD_LIST_HASH{JSON::Object::hash(KEYWORD_LIST)};
inline constexpr auto KEYWORD_NEST_HASH{JSON::Object::hash(KEYWORD_NEST)};
inline constexpr auto KEYWORD_NONE_HASH{JSON::Object::hash(KEYWORD_NONE)};
inline constexpr auto KEYWORD_PREFIX_HASH{JSON::Object::hash(KEYWORD_PREFIX)};
inline constexpr auto KEYWORD_PROPAGATE_HASH{
    JSON::Object::hash(KEYWORD_PROPAGATE)};
inline constexpr auto KEYWORD_PROTECTED_HASH{
    JSON::Object::hash(KEYWORD_PROTECTED)};
inline constexpr auto KEYWORD_REVERSE_HASH{JSON::Object::hash(KEYWORD_REVERSE)};
inline constexpr auto KEYWORD_SET_HASH{JSON::Object::hash(KEYWORD_SET)};
inline constexpr auto KEYWORD_TYPE_HASH{JSON::Object::hash(KEYWORD_TYPE)};
inline constexpr auto KEYWORD_VALUE_HASH{JSON::Object::hash(KEYWORD_VALUE)};
inline constexpr auto KEYWORD_VERSION_HASH{JSON::Object::hash(KEYWORD_VERSION)};
inline constexpr auto KEYWORD_VOCAB_HASH{JSON::Object::hash(KEYWORD_VOCAB)};

// Markers used internally by inverse context creation, term selection, and node
// map generation. These are not JSON-LD document keywords: @default is the name
// the API algorithms give the default graph, and it is defined by the framing
// specification rather than the core keyword grammar.
inline constexpr JSON::StringView KEYWORD_ANY{"@any"};
inline constexpr JSON::StringView KEYWORD_DEFAULT{"@default"};
inline constexpr JSON::StringView KEYWORD_NULL{"@null"};
inline constexpr auto KEYWORD_ANY_HASH{JSON::Object::hash(KEYWORD_ANY)};
inline constexpr auto KEYWORD_DEFAULT_HASH{JSON::Object::hash(KEYWORD_DEFAULT)};

// A stable owned copy of the @context keyword, suitable as a JSON Pointer
// token. (A namespace-scope JSON::String constant would trip clang-tidy's
// throwing-static-initialization check, hence the function-local static.)
inline auto keyword_context() -> const JSON::String & {
  static const JSON::String value{KEYWORD_CONTEXT};
  return value;
}

inline auto is_keyword(const JSON::StringView value) -> bool {
  if (value.size() < 2 || value.front() != '@') {
    return false;
  }
  return value == KEYWORD_BASE || value == KEYWORD_CONTAINER ||
         value == KEYWORD_CONTEXT || value == KEYWORD_DIRECTION ||
         value == KEYWORD_GRAPH || value == KEYWORD_ID ||
         value == KEYWORD_IMPORT || value == KEYWORD_INCLUDED ||
         value == KEYWORD_INDEX || value == KEYWORD_JSON ||
         value == KEYWORD_LANGUAGE || value == KEYWORD_LIST ||
         value == KEYWORD_NEST || value == KEYWORD_NONE ||
         value == KEYWORD_PREFIX || value == KEYWORD_PROPAGATE ||
         value == KEYWORD_PROTECTED || value == KEYWORD_REVERSE ||
         value == KEYWORD_SET || value == KEYWORD_TYPE ||
         value == KEYWORD_VALUE || value == KEYWORD_VERSION ||
         value == KEYWORD_VOCAB;
}

inline auto is_keyword(const JSON::StringView value,
                       const JSON::Object::hash_type hash) -> bool {
  if (value.size() < 2 || value.front() != '@') {
    return false;
  }
  return hash == KEYWORD_BASE_HASH || hash == KEYWORD_CONTAINER_HASH ||
         hash == KEYWORD_CONTEXT_HASH || hash == KEYWORD_DIRECTION_HASH ||
         hash == KEYWORD_GRAPH_HASH || hash == KEYWORD_ID_HASH ||
         hash == KEYWORD_IMPORT_HASH || hash == KEYWORD_INCLUDED_HASH ||
         hash == KEYWORD_INDEX_HASH || hash == KEYWORD_JSON_HASH ||
         hash == KEYWORD_LANGUAGE_HASH || hash == KEYWORD_LIST_HASH ||
         hash == KEYWORD_NEST_HASH || hash == KEYWORD_NONE_HASH ||
         hash == KEYWORD_PREFIX_HASH || hash == KEYWORD_PROPAGATE_HASH ||
         hash == KEYWORD_PROTECTED_HASH || hash == KEYWORD_REVERSE_HASH ||
         hash == KEYWORD_SET_HASH || hash == KEYWORD_TYPE_HASH ||
         hash == KEYWORD_VALUE_HASH || hash == KEYWORD_VERSION_HASH ||
         hash == KEYWORD_VOCAB_HASH;
}

// Whether the given value has the generic form of a keyword (an `@` followed by
// one or more letters), which the algorithms treat as a reserved token even
// when it is not a defined keyword.
inline auto has_keyword_form(const JSON::StringView value) -> bool {
  if (value.size() < 2 || value.front() != '@') {
    return false;
  }
  for (std::size_t index{1}; index < value.size(); index += 1) {
    if (!is_alpha(value[index])) {
      return false;
    }
  }
  return true;
}

} // namespace sourcemeta::core

#endif
