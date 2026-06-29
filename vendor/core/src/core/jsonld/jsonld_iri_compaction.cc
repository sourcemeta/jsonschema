#include <sourcemeta/core/json.h>
#include <sourcemeta/core/text.h>
#include <sourcemeta/core/uri.h>

#include "jsonld_algorithms.h"
#include "jsonld_keywords.h"

#include <optional> // std::optional, std::nullopt
#include <vector>   // std::vector

namespace sourcemeta::core {

namespace {

// The text module only lowercases in place, so this returns a lowercased copy.
auto lowercase(const JSON::StringView value) -> JSON::String {
  JSON::String result{value};
  to_lowercase(result);
  return result;
}

// Term Selection (JSON-LD 1.1 API Section 4.3.4)
auto term_selection(const JSON &inverse_context, const JSON::String &variable,
                    const std::vector<JSON::String> &containers,
                    const JSON::StringView type_language,
                    const std::vector<JSON::String> &preferred)
    -> std::optional<JSON::String> {
  const auto &container_map{inverse_context.at(variable)};
  for (const auto &container : containers) {
    if (!container_map.defines(container)) {
      continue;
    }
    const auto &type_language_map{container_map.at(container)};
    const auto &value_map{type_language_map.at(type_language)};
    for (const auto &item : preferred) {
      if (value_map.defines(item)) {
        return value_map.at(item).to_string();
      }
    }
  }
  return std::nullopt;
}

// Relativise an absolute IRI against the base IRI.
auto relativize(const JSON::String &iri, const JSON::String &base)
    -> JSON::String {
  const auto reference{URI::from_iri(base)};
  auto relative{URI::from_iri(iri).relative_to(reference).recompose()};
  // An IRI equal to the base relativises to an empty reference, but JSON-LD
  // expresses it as the base's last path segment.
  if (relative.empty()) {
    if (!reference.path().has_value()) {
      return relative;
    }
    const auto path{reference.path().value()};
    const auto slash{path.rfind('/')};
    return JSON::String{
        slash == JSON::StringView::npos ? path : path.substr(slash + 1)};
  }
  // Guard a keyword-like first segment so it is not read back as a keyword.
  if (relative.front() == '@') {
    return JSON::String{"./"} + relative;
  }
  return relative;
}

} // namespace

auto compact_iri(const ActiveContext &active_context,
                 const JSON &inverse_context, const JSON::String &variable,
                 const JSON *const value, const bool vocabulary,
                 const bool reverse) -> JSON::String {
  if (variable.empty()) {
    return variable;
  }

  if (vocabulary && inverse_context.defines(variable)) {
    // When a default base direction is set, the default language is the
    // language and direction joined by an underscore, even if there is no
    // default language (Section 6.2.3).
    JSON::String default_language{KEYWORD_NONE};
    if (active_context.default_direction.has_value()) {
      default_language =
          active_context.default_language.has_value()
              ? lowercase(active_context.default_language.value())
              : JSON::String{};
      default_language += "_";
      default_language += lowercase(active_context.default_direction.value());
    } else if (active_context.default_language.has_value()) {
      default_language = lowercase(active_context.default_language.value());
    }

    std::vector<JSON::String> containers;
    JSON::String type_language{KEYWORD_LANGUAGE};
    JSON::String type_language_value{KEYWORD_NULL};

    const bool is_object{value != nullptr && value->is_object()};
    const bool has_index{is_object &&
                         value->defines(KEYWORD_INDEX, KEYWORD_INDEX_HASH)};
    const bool is_graph{is_object &&
                        value->defines(KEYWORD_GRAPH, KEYWORD_GRAPH_HASH)};

    if (has_index && !is_graph) {
      containers.emplace_back(KEYWORD_INDEX);
      containers.emplace_back(JSON::String{KEYWORD_INDEX} +
                              JSON::String{KEYWORD_SET});
    }

    if (reverse) {
      type_language = KEYWORD_TYPE;
      type_language_value = KEYWORD_REVERSE;
      containers.emplace_back(KEYWORD_SET);
      containers.emplace_back(KEYWORD_NONE);
      // A reverse value that is a node without an explicit index may still
      // match a reverse term that declares an index container.
      if (!has_index) {
        containers.emplace_back(KEYWORD_INDEX);
        containers.emplace_back(JSON::String{KEYWORD_INDEX} +
                                JSON::String{KEYWORD_SET});
      }
    } else if (is_graph) {
      const JSON::String graph{KEYWORD_GRAPH};
      const JSON::String set{KEYWORD_SET};
      // A graph object may match any graph container the term declares, mapping
      // by @id or @index when present and by @none otherwise.
      containers.emplace_back(graph + JSON::String{KEYWORD_ID});
      containers.emplace_back(graph + JSON::String{KEYWORD_ID} + set);
      containers.emplace_back(graph + JSON::String{KEYWORD_INDEX});
      containers.emplace_back(graph + JSON::String{KEYWORD_INDEX} + set);
      containers.emplace_back(graph);
      containers.emplace_back(graph + set);
      if (has_index) {
        containers.emplace_back(KEYWORD_INDEX);
        containers.emplace_back(JSON::String{KEYWORD_INDEX} + set);
      }
      containers.emplace_back(KEYWORD_SET);
      containers.emplace_back(KEYWORD_NONE);
      type_language = KEYWORD_TYPE;
      type_language_value = KEYWORD_ID;
    } else if (is_object && value->defines(KEYWORD_LIST, KEYWORD_LIST_HASH)) {
      if (!has_index) {
        containers.emplace_back(KEYWORD_LIST);
      }
      const auto &list{value->at(KEYWORD_LIST, KEYWORD_LIST_HASH)};
      std::optional<JSON::String> common_type;
      std::optional<JSON::String> common_language;
      if (list.empty()) {
        common_language = default_language;
      }
      for (const auto &item : list.as_array()) {
        JSON::String item_language{KEYWORD_NONE};
        JSON::String item_type{KEYWORD_NONE};
        if (item.is_object() &&
            item.defines(KEYWORD_VALUE, KEYWORD_VALUE_HASH)) {
          const bool item_has_language{
              item.defines(KEYWORD_LANGUAGE, KEYWORD_LANGUAGE_HASH)};
          const bool item_has_direction{
              item.defines(KEYWORD_DIRECTION, KEYWORD_DIRECTION_HASH)};
          if (item_has_language && item_has_direction) {
            item_language =
                lowercase(item.at(KEYWORD_LANGUAGE, KEYWORD_LANGUAGE_HASH)
                              .to_string()) +
                "_" +
                lowercase(item.at(KEYWORD_DIRECTION, KEYWORD_DIRECTION_HASH)
                              .to_string());
          } else if (item_has_language) {
            item_language = lowercase(
                item.at(KEYWORD_LANGUAGE, KEYWORD_LANGUAGE_HASH).to_string());
          } else if (item_has_direction) {
            item_language = "_" + lowercase(item.at(KEYWORD_DIRECTION,
                                                    KEYWORD_DIRECTION_HASH)
                                                .to_string());
          } else if (item.defines(KEYWORD_TYPE, KEYWORD_TYPE_HASH)) {
            item_type = item.at(KEYWORD_TYPE, KEYWORD_TYPE_HASH).to_string();
          } else {
            item_language = KEYWORD_NULL;
          }
        } else {
          item_type = KEYWORD_ID;
        }
        if (!common_language.has_value()) {
          common_language = item_language;
        } else if (common_language.value() != item_language &&
                   item.is_object() &&
                   item.defines(KEYWORD_VALUE, KEYWORD_VALUE_HASH)) {
          common_language = JSON::String{KEYWORD_NONE};
        }
        if (!common_type.has_value()) {
          common_type = item_type;
        } else if (common_type.value() != item_type) {
          common_type = JSON::String{KEYWORD_NONE};
        }
      }
      if (!common_language.has_value()) {
        common_language = JSON::String{KEYWORD_NONE};
      }
      if (!common_type.has_value()) {
        common_type = JSON::String{KEYWORD_NONE};
      }
      if (common_type.value() != KEYWORD_NONE) {
        type_language = KEYWORD_TYPE;
        type_language_value = common_type.value();
      } else {
        type_language_value = common_language.value();
      }
      // An empty list matches a term via the @any fallback.
      if (list.empty()) {
        type_language = KEYWORD_ANY;
      }
      // Fall back to an exact-match term when no list-container term applies.
      containers.emplace_back(KEYWORD_NONE);
    } else if (is_object && value->defines(KEYWORD_VALUE, KEYWORD_VALUE_HASH)) {
      const bool has_language{
          value->defines(KEYWORD_LANGUAGE, KEYWORD_LANGUAGE_HASH)};
      const bool has_direction{
          value->defines(KEYWORD_DIRECTION, KEYWORD_DIRECTION_HASH)};
      const JSON::String language_set{JSON::String{KEYWORD_LANGUAGE} +
                                      JSON::String{KEYWORD_SET}};
      if (has_direction && !has_index) {
        type_language_value =
            (has_language
                 ? lowercase(value->at(KEYWORD_LANGUAGE, KEYWORD_LANGUAGE_HASH)
                                 .to_string())
                 : JSON::String{}) +
            "_" +
            lowercase(value->at(KEYWORD_DIRECTION, KEYWORD_DIRECTION_HASH)
                          .to_string());
        containers.emplace_back(KEYWORD_LANGUAGE);
        containers.emplace_back(language_set);
      } else if (has_language && !has_index) {
        type_language_value = lowercase(
            value->at(KEYWORD_LANGUAGE, KEYWORD_LANGUAGE_HASH).to_string());
        containers.emplace_back(KEYWORD_LANGUAGE);
        containers.emplace_back(language_set);
      } else if (value->defines(KEYWORD_TYPE, KEYWORD_TYPE_HASH)) {
        type_language = KEYWORD_TYPE;
        type_language_value =
            value->at(KEYWORD_TYPE, KEYWORD_TYPE_HASH).to_string();
      }
      containers.emplace_back(KEYWORD_SET);
      containers.emplace_back(KEYWORD_NONE);
      if (!has_index) {
        containers.emplace_back(KEYWORD_INDEX);
        containers.emplace_back(JSON::String{KEYWORD_INDEX} +
                                JSON::String{KEYWORD_SET});
      }
      // A value object carrying only @value also matches a language container
      // term as a last resort, after a no-container term has been considered.
      if (value->object_size() == 1) {
        containers.emplace_back(KEYWORD_LANGUAGE);
        containers.emplace_back(language_set);
      }
    } else {
      type_language = KEYWORD_TYPE;
      type_language_value = KEYWORD_ID;
      containers.emplace_back(KEYWORD_ID);
      containers.emplace_back(JSON::String{KEYWORD_ID} +
                              JSON::String{KEYWORD_SET});
      containers.emplace_back(KEYWORD_TYPE);
      // The inverse context stores container keys sorted, so @set precedes
      // @type.
      containers.emplace_back(JSON::String{KEYWORD_SET} +
                              JSON::String{KEYWORD_TYPE});
      containers.emplace_back(KEYWORD_INDEX);
      containers.emplace_back(JSON::String{KEYWORD_INDEX} +
                              JSON::String{KEYWORD_SET});
      containers.emplace_back(KEYWORD_SET);
      containers.emplace_back(KEYWORD_NONE);
    }

    std::vector<JSON::String> preferred;
    if (type_language_value == KEYWORD_REVERSE) {
      preferred.emplace_back(KEYWORD_REVERSE);
    }
    if ((type_language_value == KEYWORD_ID ||
         type_language_value == KEYWORD_REVERSE) &&
        is_object && value->defines(KEYWORD_ID, KEYWORD_ID_HASH)) {
      const auto &identifier{value->at(KEYWORD_ID, KEYWORD_ID_HASH)};
      if (identifier.is_string()) {
        const auto compacted_id{compact_iri(active_context, inverse_context,
                                            identifier.to_string(), nullptr,
                                            true, false)};
        const auto iterator{active_context.terms.find(compacted_id)};
        if (iterator != active_context.terms.cend() &&
            iterator->second.iri.has_value() &&
            iterator->second.iri.value() == identifier.to_string()) {
          preferred.emplace_back(KEYWORD_VOCAB);
          preferred.emplace_back(KEYWORD_ID);
          preferred.emplace_back(KEYWORD_NONE);
        } else {
          preferred.emplace_back(KEYWORD_ID);
          preferred.emplace_back(KEYWORD_VOCAB);
          preferred.emplace_back(KEYWORD_NONE);
        }
      }
    } else if (value == nullptr) {
      // A bare vocabulary reference may match a term that coerces @id or
      // @vocab.
      preferred.emplace_back(KEYWORD_ID);
      preferred.emplace_back(KEYWORD_VOCAB);
      preferred.emplace_back(KEYWORD_NONE);
    } else {
      preferred.emplace_back(type_language_value);
      preferred.emplace_back(KEYWORD_NONE);
    }
    preferred.emplace_back(KEYWORD_ANY);

    // A language-and-direction preferred value also allows matching a term that
    // only coerces direction, via the substring from the underscore onwards.
    std::vector<JSON::String> direction_fallbacks;
    for (const auto &item : preferred) {
      const auto underscore{item.find('_')};
      if (underscore != JSON::String::npos) {
        direction_fallbacks.emplace_back(item.substr(underscore));
      }
    }
    for (auto &fallback : direction_fallbacks) {
      preferred.push_back(std::move(fallback));
    }

    const auto term{term_selection(inverse_context, variable, containers,
                                   type_language, preferred)};
    if (term.has_value()) {
      return term.value();
    }
  }

  // Compact relative to @vocab.
  if (vocabulary && active_context.vocabulary.has_value()) {
    const auto &vocab{active_context.vocabulary.value()};
    if (variable.size() > vocab.size() && variable.starts_with(vocab)) {
      const auto suffix{variable.substr(vocab.size())};
      if (active_context.terms.find(suffix) == active_context.terms.cend()) {
        return JSON::String{suffix};
      }
    }
  }

  // Compact into a prefix:suffix compact IRI.
  std::optional<JSON::String> candidate;
  for (const auto &entry : active_context.terms) {
    const auto &term{entry.first};
    const auto &definition{entry.second};
    if (!definition.iri.has_value() || definition.iri.value() == variable ||
        !definition.prefix || !variable.starts_with(definition.iri.value())) {
      continue;
    }
    auto compact{term};
    compact += ":";
    compact += variable.substr(definition.iri.value().size());
    const auto existing{active_context.terms.find(compact)};
    const bool usable{existing == active_context.terms.cend() ||
                      (value == nullptr && existing->second.iri.has_value() &&
                       existing->second.iri.value() == variable)};
    if (usable &&
        (!candidate.has_value() || compact.size() < candidate.value().size() ||
         (compact.size() == candidate.value().size() &&
          compact < candidate.value()))) {
      candidate = compact;
    }
  }
  if (candidate.has_value()) {
    return candidate.value();
  }

  // An absolute IRI whose scheme matches a prefix term and that has no
  // authority would be misread as a compact IRI (Section 6.2.3).
  const auto colon{variable.find(':')};
  if (colon != JSON::String::npos &&
      variable.compare(colon + 1, 2, "//") != 0) {
    const auto scheme{variable.substr(0, colon)};
    const auto iterator{active_context.terms.find(scheme)};
    if (iterator != active_context.terms.cend() && iterator->second.prefix) {
      throw JSONLDError("IRI confused with prefix", empty_weak_pointer);
    }
  }

  // Relativise an absolute IRI against the base IRI.
  if (!vocabulary && active_context.compact_to_relative &&
      active_context.base.has_value() && URI::is_iri(variable)) {
    return relativize(variable, active_context.base.value());
  }

  return variable;
}

} // namespace sourcemeta::core
