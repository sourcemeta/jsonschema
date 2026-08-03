#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonld.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/langtag.h>
#include <sourcemeta/core/uri.h>

#include "jsonld_algorithms.h"
#include "jsonld_keywords.h"
#include "jsonld_serialise.h"

#include <algorithm>  // std::ranges::sort, std::ranges::find
#include <cstddef>    // std::size_t
#include <functional> // std::reference_wrapper, std::cref
#include <optional>   // std::optional, std::nullopt
#include <utility>    // std::move
#include <vector>     // std::vector

namespace sourcemeta::core {

namespace {

// Node types and identifiers have their own descriptor channels, so a
// fragment cannot smuggle them in as properties, including through the
// expanded spelling of the type keyword as a plain predicate
constexpr JSON::StringView PREDICATE_RDF_TYPE{
    "http://www.w3.org/1999/02/22-rdf-syntax-ns#type"};

auto validate_key(const JSON::String &key) -> void {
  if (key == KEYWORD_TYPE || key == PREDICATE_RDF_TYPE) {
    throw JSONLDFragmentError{"A constants fragment cannot declare node types",
                              Pointer{key}, JSON::String{key}};
  }
  if (key == KEYWORD_ID) {
    throw JSONLDFragmentError{
        "A constants fragment cannot declare a node identifier", Pointer{key},
        JSON::String{key}};
  }
  if (!key.empty() && key.front() == '@') {
    throw JSONLDFragmentError{"A constants fragment key cannot be a keyword",
                              Pointer{key}, JSON::String{key}};
  }
  if (!URI::is_iri(key)) {
    throw JSONLDFragmentError{"A constants fragment key must be an absolute "
                              "IRI",
                              Pointer{key}, JSON::String{key}};
  }
}

// The location of a fragment term, only materialized on the error path so
// that valid fragments never pay for pointer construction
auto term_location(const JSON::String &key,
                   const std::optional<std::size_t> index) -> Pointer {
  if (index.has_value()) {
    return Pointer{key, index.value()};
  }

  return Pointer{key};
}

// The fragment grammar reuses the expanded form conventions of JSON-LD 1.1
// Section 9, so a bare string is always a string literal and a node
// reference is spelled with an explicit identifier entry
auto canonicalize_term(const JSON::String &key,
                       const std::optional<std::size_t> index, const JSON &term)
    -> JSON {
  if (term.is_string() || term.is_boolean() || term.is_number()) {
    auto result{JSON::make_object()};
    result.assign_assume_new(JSON::String{KEYWORD_VALUE}, JSON{term},
                             KEYWORD_VALUE_HASH);
    return result;
  }

  if (!term.is_object()) {
    throw JSONLDFragmentError{"A constants fragment term must be a scalar, a "
                              "node reference, or a value object",
                              term_location(key, index), JSON::String{key}};
  }

  if (term.defines(KEYWORD_ID, KEYWORD_ID_HASH)) {
    if (term.object_size() != 1) {
      throw JSONLDFragmentError{"A node reference can only carry an identifier",
                                term_location(key, index), JSON::String{key}};
    }
    const auto &identifier{term.at(KEYWORD_ID, KEYWORD_ID_HASH)};
    if (!identifier.is_string() || is_blank_node(identifier.to_string()) ||
        !URI::is_iri(identifier.to_string())) {
      throw JSONLDFragmentError{
          "A node reference identifier must be an absolute IRI",
          term_location(key, index), JSON::String{key}};
    }
    return JSON{term};
  }

  if (!term.defines(KEYWORD_VALUE, KEYWORD_VALUE_HASH)) {
    throw JSONLDFragmentError{"A constants fragment term must be a scalar, a "
                              "node reference, or a value object",
                              term_location(key, index), JSON::String{key}};
  }

  for (const auto &entry : term.as_object()) {
    if (entry.first != KEYWORD_VALUE && entry.first != KEYWORD_TYPE &&
        entry.first != KEYWORD_LANGUAGE) {
      throw JSONLDFragmentError{
          "A value object can only carry a value, a type, and a language",
          term_location(key, index), JSON::String{key}};
    }
  }

  const auto &value{term.at(KEYWORD_VALUE, KEYWORD_VALUE_HASH)};
  if (!value.is_string() && !value.is_boolean() && !value.is_number()) {
    throw JSONLDFragmentError{"A value object value must be a non-null scalar",
                              term_location(key, index), JSON::String{key}};
  }

  // A type excludes a language and vice versa (JSON-LD 1.1 Section 9.5, "a
  // value object must not contain both a type and a language member")
  const bool has_type{term.defines(KEYWORD_TYPE, KEYWORD_TYPE_HASH)};
  const bool has_language{
      term.defines(KEYWORD_LANGUAGE, KEYWORD_LANGUAGE_HASH)};
  if (has_type && has_language) {
    throw JSONLDFragmentError{
        "A value object cannot combine a type and a language",
        term_location(key, index), JSON::String{key}};
  }

  if (has_language) {
    if (!value.is_string()) {
      throw JSONLDFragmentError{"A value object language requires a string "
                                "value",
                                term_location(key, index), JSON::String{key}};
    }
    const auto &language{term.at(KEYWORD_LANGUAGE, KEYWORD_LANGUAGE_HASH)};
    if (!language.is_string() || !is_canonical_langtag(language.to_string())) {
      throw JSONLDFragmentError{
          "A value object language must be a canonical BCP 47 language tag",
          term_location(key, index), JSON::String{key}};
    }
  }

  if (has_type) {
    const auto &datatype{term.at(KEYWORD_TYPE, KEYWORD_TYPE_HASH)};
    if (!datatype.is_string()) {
      throw JSONLDFragmentError{"A value object type must be an absolute IRI",
                                term_location(key, index), JSON::String{key}};
    }
    // The JSON literal spelling is checked before the IRI shape, as it is
    // not an IRI and would otherwise report the wrong rule
    if (datatype.to_string() == KEYWORD_JSON) {
      throw JSONLDFragmentError{
          "A value object type cannot be the JSON literal type",
          term_location(key, index), JSON::String{key}};
    }
    if (!URI::is_iri(datatype.to_string())) {
      throw JSONLDFragmentError{"A value object type must be an absolute IRI",
                                term_location(key, index), JSON::String{key}};
    }

    // A native number or boolean under an explicit datatype converts to RDF
    // through a canonical string lexical form (JSON-LD 1.1 API Section 8.6),
    // so the canonical fragment stores that form up front. Strings yield no
    // lexical form and pass through verbatim
    auto lexical{typed_literal_lexical_form(value, datatype.to_string())};
    if (lexical.has_value()) {
      auto rewritten{JSON::make_object()};
      rewritten.assign_assume_new(JSON::String{KEYWORD_VALUE},
                                  JSON{std::move(lexical).value()},
                                  KEYWORD_VALUE_HASH);
      rewritten.assign_assume_new(JSON::String{KEYWORD_TYPE}, JSON{datatype},
                                  KEYWORD_TYPE_HASH);
      return rewritten;
    }
  }

  return JSON{term};
}

auto canonicalize_entry(const JSON::String &key, const JSON &entry) -> JSON {
  auto terms{JSON::make_array()};
  if (entry.is_array()) {
    for (std::size_t index = 0; index < entry.size(); index += 1) {
      auto term{canonicalize_term(key, index, entry.at(index))};
      if (std::ranges::find(terms.as_array(), term) ==
          terms.as_array().cend()) {
        terms.push_back(std::move(term));
      }
    }
  } else {
    terms.push_back(canonicalize_term(key, std::nullopt, entry));
  }

  return terms;
}

} // namespace

auto jsonld_canonicalize_fragment(const JSON &fragment) -> JSON {
  if (!fragment.is_object()) {
    throw JSONLDFragmentError{"A constants fragment must be an object",
                              Pointer{}, JSON::String{}};
  }

  std::vector<std::reference_wrapper<const JSON::String>> keys;
  keys.reserve(fragment.object_size());
  for (const auto &entry : fragment.as_object()) {
    keys.push_back(std::cref(entry.first));
  }
  std::ranges::sort(keys, [](const auto &left, const auto &right) -> bool {
    return left.get() < right.get();
  });

  auto result{JSON::make_object()};
  for (const auto key : keys) {
    validate_key(key.get());
    const auto &entry{fragment.at(key.get())};

    // A null entry is the caller's removal channel and passes through
    // untouched, and an empty array asserts nothing so its key is dropped
    if (entry.is_null()) {
      result.assign_assume_new(JSON::String{key.get()}, JSON{nullptr});
      continue;
    }
    if (entry.is_array() && entry.empty()) {
      continue;
    }

    result.assign_assume_new(JSON::String{key.get()},
                             canonicalize_entry(key.get(), entry));
  }

  return result;
}

} // namespace sourcemeta::core
