#include <sourcemeta/core/json.h>
#include <sourcemeta/core/text.h>

#include "jsonld_algorithms.h"
#include "jsonld_keywords.h"

#include <algorithm> // std::sort
#include <vector>    // std::vector

namespace sourcemeta::core {

namespace {

// The text module only lowercases in place, so this returns a lowercased copy.
auto lowercase(const JSON::StringView value) -> JSON::String {
  JSON::String result{value};
  to_lowercase(result);
  return result;
}

auto assign_if_missing(JSON &map, const JSON::StringView key,
                       const JSON::String &term) -> void {
  if (!map.defines(key)) {
    map.assign_assume_new(JSON::String{key}, JSON{term});
  }
}

} // namespace

auto create_inverse_context(const ActiveContext &active_context) -> JSON {
  auto result{JSON::make_object()};

  // When a default base direction is set, the default language is the language
  // and direction joined by an underscore, even if there is no default
  // language (Section 4.3.1).
  JSON::String default_language{KEYWORD_NONE};
  if (active_context.default_direction.has_value()) {
    default_language = active_context.default_language.has_value()
                           ? lowercase(active_context.default_language.value())
                           : JSON::String{};
    default_language += "_";
    default_language += lowercase(active_context.default_direction.value());
  } else if (active_context.default_language.has_value()) {
    default_language = lowercase(active_context.default_language.value());
  }

  // Terms are processed shortest first, breaking ties lexicographically, so
  // that shorter and earlier terms win when several map to the same IRI.
  std::vector<JSON::String> terms;
  terms.reserve(active_context.terms.size());
  for (const auto &entry : active_context.terms) {
    terms.push_back(entry.first);
  }
  std::ranges::sort(
      terms, [](const JSON::String &left, const JSON::String &right) -> bool {
        if (left.size() != right.size()) {
          return left.size() < right.size();
        }
        return left < right;
      });

  for (const auto &term : terms) {
    const auto &definition{active_context.terms.at(term)};
    if (!definition.iri.has_value()) {
      continue;
    }

    JSON::String container{KEYWORD_NONE};
    if (!definition.container.empty()) {
      auto ordered{definition.container};
      std::ranges::sort(ordered);
      container.clear();
      for (const auto &item : ordered) {
        container += item;
      }
    }

    const auto &variable{definition.iri.value()};
    if (!result.defines(variable)) {
      result.assign_assume_new(variable, JSON::make_object());
    }
    auto &container_map{result.at(variable)};
    if (!container_map.defines(container)) {
      auto type_language_map{JSON::make_object()};
      type_language_map.assign_assume_new(JSON::String{KEYWORD_LANGUAGE},
                                          JSON::make_object(),
                                          KEYWORD_LANGUAGE_HASH);
      type_language_map.assign_assume_new(
          JSON::String{KEYWORD_TYPE}, JSON::make_object(), KEYWORD_TYPE_HASH);
      auto any_map{JSON::make_object()};
      any_map.assign_assume_new(JSON::String{KEYWORD_NONE}, JSON{term},
                                KEYWORD_NONE_HASH);
      type_language_map.assign_assume_new(JSON::String{KEYWORD_ANY},
                                          std::move(any_map), KEYWORD_ANY_HASH);
      container_map.assign_assume_new(container, std::move(type_language_map));
    }
    auto &type_language_map{container_map.at(container)};
    auto &type_map{type_language_map.at(KEYWORD_TYPE, KEYWORD_TYPE_HASH)};
    auto &language_map{
        type_language_map.at(KEYWORD_LANGUAGE, KEYWORD_LANGUAGE_HASH)};

    if (definition.reverse) {
      assign_if_missing(type_map, KEYWORD_REVERSE, term);
    } else if (definition.type_mapping.has_value() &&
               definition.type_mapping.value() == KEYWORD_NONE) {
      assign_if_missing(language_map, KEYWORD_ANY, term);
      assign_if_missing(type_map, KEYWORD_ANY, term);
    } else if (definition.type_mapping.has_value()) {
      assign_if_missing(type_map, definition.type_mapping.value(), term);
    } else if (definition.has_language && definition.has_direction) {
      JSON::String key;
      if (definition.language.has_value() && definition.direction.has_value()) {
        key = lowercase(definition.language.value());
        key += "_";
        key += lowercase(definition.direction.value());
      } else if (definition.language.has_value()) {
        key = lowercase(definition.language.value());
      } else if (definition.direction.has_value()) {
        key = "_";
        key += lowercase(definition.direction.value());
      } else {
        key = KEYWORD_NULL;
      }
      assign_if_missing(language_map, key, term);
    } else if (definition.has_language) {
      const JSON::String key{definition.language.has_value()
                                 ? lowercase(definition.language.value())
                                 : JSON::String{KEYWORD_NULL}};
      assign_if_missing(language_map, key, term);
    } else if (definition.has_direction) {
      JSON::String key{KEYWORD_NONE};
      if (definition.direction.has_value()) {
        key = "_";
        key += lowercase(definition.direction.value());
      }
      assign_if_missing(language_map, key, term);
    } else {
      assign_if_missing(language_map, default_language, term);
      assign_if_missing(language_map, KEYWORD_NONE, term);
      assign_if_missing(type_map, KEYWORD_NONE, term);
    }
  }

  return result;
}

} // namespace sourcemeta::core
