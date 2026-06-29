#include "jsonld_algorithms.h"
#include "jsonld_keywords.h"

#include <optional> // std::optional

namespace sourcemeta::core {

// Value Expansion (JSON-LD 1.1 API Section 5.3.3)
auto expand_value(ExpansionState &state, ActiveContext &active_context,
                  const std::optional<JSON::String> &active_property,
                  const JSON &value) -> JSON {
  const TermDefinition *definition{nullptr};
  if (active_property.has_value()) {
    const auto iterator{active_context.terms.find(active_property.value())};
    if (iterator != active_context.terms.cend()) {
      definition = &iterator->second;
    }
  }

  if (definition != nullptr && definition->type_mapping.has_value() &&
      value.is_string()) {
    if (definition->type_mapping.value() == KEYWORD_ID) {
      auto result{JSON::make_object()};
      const auto identifier{expand_iri(state, active_context, value.to_string(),
                                       true, false, nullptr, nullptr,
                                       empty_weak_pointer)};
      result.assign_assume_new(JSON::String{KEYWORD_ID},
                               identifier.has_value() ? JSON{identifier.value()}
                                                      : JSON{nullptr},
                               KEYWORD_ID_HASH);
      return result;
    }
    if (definition->type_mapping.value() == KEYWORD_VOCAB) {
      auto result{JSON::make_object()};
      const auto identifier{expand_iri(state, active_context, value.to_string(),
                                       true, true, nullptr, nullptr,
                                       empty_weak_pointer)};
      result.assign_assume_new(JSON::String{KEYWORD_ID},
                               identifier.has_value() ? JSON{identifier.value()}
                                                      : JSON{nullptr},
                               KEYWORD_ID_HASH);
      return result;
    }
  }

  auto result{JSON::make_object()};
  result.assign_assume_new(JSON::String{KEYWORD_VALUE}, JSON{value},
                           KEYWORD_VALUE_HASH);

  if (definition != nullptr && definition->type_mapping.has_value() &&
      definition->type_mapping.value() != KEYWORD_ID &&
      definition->type_mapping.value() != KEYWORD_VOCAB &&
      definition->type_mapping.value() != KEYWORD_NONE) {
    result.assign_assume_new(JSON::String{KEYWORD_TYPE},
                             JSON{definition->type_mapping.value()},
                             KEYWORD_TYPE_HASH);
  } else if (value.is_string()) {
    const auto language{definition != nullptr && definition->has_language
                            ? definition->language
                            : active_context.default_language};
    if (language.has_value()) {
      result.assign_assume_new(JSON::String{KEYWORD_LANGUAGE},
                               JSON{language.value()}, KEYWORD_LANGUAGE_HASH);
    }
    const auto direction{definition != nullptr && definition->has_direction
                             ? definition->direction
                             : active_context.default_direction};
    if (direction.has_value()) {
      result.assign_assume_new(JSON::String{KEYWORD_DIRECTION},
                               JSON{direction.value()}, KEYWORD_DIRECTION_HASH);
    }
  }

  return result;
}

} // namespace sourcemeta::core
