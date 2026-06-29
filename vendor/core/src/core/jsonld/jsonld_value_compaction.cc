#include <sourcemeta/core/json.h>
#include <sourcemeta/core/text.h>

#include "jsonld_algorithms.h"
#include "jsonld_keywords.h"

#include <algorithm> // std::find
#include <optional>  // std::optional

namespace sourcemeta::core {

namespace {

// The text module only lowercases in place, so this returns a lowercased copy.
auto lowercase(const JSON::StringView value) -> JSON::String {
  JSON::String result{value};
  to_lowercase(result);
  return result;
}

auto container_includes(const TermDefinition *const definition,
                        const JSON::StringView keyword) -> bool {
  if (definition == nullptr) {
    return false;
  }
  return std::ranges::find(definition->container, keyword) !=
         definition->container.cend();
}

} // namespace

auto compact_value(const ActiveContext &active_context,
                   const JSON &inverse_context,
                   const std::optional<JSON::String> &active_property,
                   const JSON &value) -> JSON {
  const TermDefinition *definition{nullptr};
  if (active_property.has_value()) {
    const auto iterator{active_context.terms.find(active_property.value())};
    if (iterator != active_context.terms.cend()) {
      definition = &iterator->second;
    }
  }

  const std::optional<JSON::String> language{
      definition != nullptr && definition->has_language
          ? definition->language
          : active_context.default_language};
  const std::optional<JSON::String> direction{
      definition != nullptr && definition->has_direction
          ? definition->direction
          : active_context.default_direction};
  const bool index_ok{!value.defines(KEYWORD_INDEX, KEYWORD_INDEX_HASH) ||
                      container_includes(definition, KEYWORD_INDEX)};

  if (value.defines(KEYWORD_ID, KEYWORD_ID_HASH)) {
    const auto extra{
        value.object_size() - 1 -
        (value.defines(KEYWORD_INDEX, KEYWORD_INDEX_HASH) ? 1U : 0U)};
    const auto &identifier{value.at(KEYWORD_ID, KEYWORD_ID_HASH)};
    if (extra == 0 && index_ok && definition != nullptr &&
        definition->type_mapping.has_value() && identifier.is_string()) {
      if (definition->type_mapping.value() == KEYWORD_ID) {
        return JSON{compact_iri(active_context, inverse_context,
                                identifier.to_string(), nullptr, false, false)};
      }
      if (definition->type_mapping.value() == KEYWORD_VOCAB) {
        return JSON{compact_iri(active_context, inverse_context,
                                identifier.to_string(), nullptr, true, false)};
      }
    }
  } else if (value.defines(KEYWORD_VALUE, KEYWORD_VALUE_HASH)) {
    const auto &contents{value.at(KEYWORD_VALUE, KEYWORD_VALUE_HASH)};
    const bool has_type{value.defines(KEYWORD_TYPE, KEYWORD_TYPE_HASH)};
    const bool type_matches{
        has_type && definition != nullptr &&
        definition->type_mapping.has_value() &&
        definition->type_mapping.value() ==
            value.at(KEYWORD_TYPE, KEYWORD_TYPE_HASH).to_string()};
    const bool type_disabled{
        (definition != nullptr && definition->type_mapping.has_value() &&
         definition->type_mapping.value() == KEYWORD_NONE) ||
        (has_type && !type_matches)};

    if (type_matches && index_ok) {
      return contents;
    } else if (type_disabled) {
      // Value compaction is disabled, but the @type IRI is still compacted in
      // the full form below.
    } else if (!contents.is_string()) {
      if (index_ok) {
        return contents;
      }
    } else {
      const bool language_matches{
          value.defines(KEYWORD_LANGUAGE, KEYWORD_LANGUAGE_HASH)
              ? (language.has_value() &&
                 lowercase(value.at(KEYWORD_LANGUAGE, KEYWORD_LANGUAGE_HASH)
                               .to_string()) == lowercase(language.value()))
              : !language.has_value()};
      const bool direction_matches{
          value.defines(KEYWORD_DIRECTION, KEYWORD_DIRECTION_HASH)
              ? (direction.has_value() &&
                 value.at(KEYWORD_DIRECTION, KEYWORD_DIRECTION_HASH)
                         .to_string() == direction.value())
              : !direction.has_value()};
      if (language_matches && direction_matches && index_ok) {
        return contents;
      }
    }
  }

  // Full form: keep the value object but alias its keyword keys and compact the
  // @type and @id IRIs.
  auto result{JSON::make_object()};
  for (const auto &entry : value.as_object()) {
    const auto key{compact_iri(active_context, inverse_context, entry.first,
                               nullptr, true, false)};
    if (entry.first == KEYWORD_TYPE && entry.second.is_string()) {
      result.assign_assume_new(
          key,
          JSON{compact_iri(active_context, inverse_context,
                           entry.second.to_string(), nullptr, true, false)});
    } else if (entry.first == KEYWORD_ID && entry.second.is_string()) {
      result.assign_assume_new(
          key,
          JSON{compact_iri(active_context, inverse_context,
                           entry.second.to_string(), nullptr, false, false)});
    } else {
      result.assign_assume_new(key, JSON{entry.second});
    }
  }
  return result;
}

} // namespace sourcemeta::core
