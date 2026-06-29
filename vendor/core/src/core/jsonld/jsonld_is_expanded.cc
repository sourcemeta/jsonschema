#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonld.h>
#include <sourcemeta/core/langtag.h>
#include <sourcemeta/core/uri.h>

#include "jsonld_keywords.h"

#include <cstdint>     // std::uint8_t
#include <string_view> // std::string_view
#include <utility>     // std::pair
#include <vector>      // std::vector

namespace sourcemeta::core {

namespace {

// A blank node identifier is the prefix "_:" followed by a label (JSON-LD 1.1
// Section 3.5).
auto is_blank_node(const std::string_view value) -> bool {
  return value.size() > 2 && value.starts_with("_:");
}

// A property term in expanded form is an absolute IRI or a blank node
// identifier (JSON-LD 1.1 Section 9, "node object").
auto is_term(const std::string_view value) -> bool {
  return URI::is_iri(value) || is_blank_node(value);
}

// @id and @type carry IRI references, which may be relative, or blank node
// identifiers (JSON-LD 1.1 Section 9, "node object").
auto is_reference(const std::string_view value) -> bool {
  return URI::is_iri_reference(value) || is_blank_node(value);
}

// What a position is expected to be, so that the traversal can validate it
// without recursion: a node object, or a property array item that is a value,
// node, list, or set object.
enum class Expect : std::uint8_t { Node, Item };

using Pending = std::vector<std::pair<const JSON *, Expect>>;

// "A value object MUST NOT contain any other entries" beyond @value, @type,
// @language, @direction, and @index, and "MUST NOT contain both @type and
// @language" (JSON-LD 1.1 Section 9, "value object"). A value object is a leaf:
// it carries no positions to traverse further.
auto is_value_object(const JSON &value) -> bool {
  for (const auto &entry : value.as_object()) {
    if (!entry.key_equals(KEYWORD_VALUE, KEYWORD_VALUE_HASH) &&
        !entry.key_equals(KEYWORD_TYPE, KEYWORD_TYPE_HASH) &&
        !entry.key_equals(KEYWORD_LANGUAGE, KEYWORD_LANGUAGE_HASH) &&
        !entry.key_equals(KEYWORD_DIRECTION, KEYWORD_DIRECTION_HASH) &&
        !entry.key_equals(KEYWORD_INDEX, KEYWORD_INDEX_HASH)) {
      return false;
    }
  }

  const auto *const contents{value.try_at(KEYWORD_VALUE, KEYWORD_VALUE_HASH)};
  if (contents == nullptr) {
    return false;
  }

  const auto *const type{value.try_at(KEYWORD_TYPE, KEYWORD_TYPE_HASH)};
  const auto *const language{
      value.try_at(KEYWORD_LANGUAGE, KEYWORD_LANGUAGE_HASH)};
  const auto *const direction{
      value.try_at(KEYWORD_DIRECTION, KEYWORD_DIRECTION_HASH)};

  if (type != nullptr && (language != nullptr || direction != nullptr)) {
    return false;
  }

  // When the datatype is @json the value is preserved verbatim, otherwise it is
  // a scalar literal.
  if (type != nullptr) {
    if (!type->is_string()) {
      return false;
    }
    if (type->to_string() != KEYWORD_JSON &&
        (!URI::is_iri(type->to_string()) || contents->is_object() ||
         contents->is_array())) {
      return false;
    }
  } else if (contents->is_object() || contents->is_array()) {
    return false;
  }

  if (language != nullptr &&
      (!language->is_string() || !is_langtag(language->to_string()) ||
       !contents->is_string())) {
    return false;
  }

  if (direction != nullptr &&
      (!direction->is_string() ||
       (direction->to_string() != "ltr" && direction->to_string() != "rtl") ||
       !contents->is_string())) {
    return false;
  }

  const auto *const index{value.try_at(KEYWORD_INDEX, KEYWORD_INDEX_HASH)};
  return index == nullptr || index->is_string();
}

// Validate one node object, appending its descendant positions to the traversal
// rather than recursing. "A node object MUST NOT contain the @value, @list, or
// @set keywords" (JSON-LD 1.1 Section 9, "node object"). @id and @type carry
// IRI references, which may be relative, whereas property terms are absolute
// IRIs or blank node identifiers.
auto validate_node(const JSON &value, Pending &pending) -> bool {
  if (!value.is_object()) {
    return false;
  }

  for (const auto &entry : value.as_object()) {
    const JSON::StringView key{entry.first};
    if (!key.empty() && key.front() == '@') {
      if (entry.key_equals(KEYWORD_ID, KEYWORD_ID_HASH)) {
        if (!entry.second.is_string() ||
            !is_reference(entry.second.to_string())) {
          return false;
        }
      } else if (entry.key_equals(KEYWORD_INDEX, KEYWORD_INDEX_HASH)) {
        if (!entry.second.is_string()) {
          return false;
        }
      } else if (entry.key_equals(KEYWORD_TYPE, KEYWORD_TYPE_HASH)) {
        if (!entry.second.is_array()) {
          return false;
        }
        for (const auto &item : entry.second.as_array()) {
          if (!item.is_string() || !is_reference(item.to_string())) {
            return false;
          }
        }
      } else if (entry.key_equals(KEYWORD_GRAPH, KEYWORD_GRAPH_HASH) ||
                 entry.key_equals(KEYWORD_INCLUDED, KEYWORD_INCLUDED_HASH)) {
        if (!entry.second.is_array()) {
          return false;
        }
        for (const auto &item : entry.second.as_array()) {
          pending.emplace_back(&item, Expect::Node);
        }
      } else if (entry.key_equals(KEYWORD_REVERSE, KEYWORD_REVERSE_HASH)) {
        if (!entry.second.is_object()) {
          return false;
        }
        for (const auto &reverse : entry.second.as_object()) {
          if (!is_term(reverse.first) || !reverse.second.is_array()) {
            return false;
          }
          for (const auto &item : reverse.second.as_array()) {
            pending.emplace_back(&item, Expect::Node);
          }
        }
      } else {
        return false;
      }
    } else if (!is_term(key) || !entry.second.is_array()) {
      return false;
    } else {
      // "The value of an expanded property is an array" (JSON-LD 1.1 Section 9)
      // whose entries are value, node, list, or set objects.
      for (const auto &item : entry.second.as_array()) {
        pending.emplace_back(&item, Expect::Item);
      }
    }
  }

  return true;
}

// Validate one list or set object, appending its entries to the traversal. It
// "MUST NOT contain any other entries" beyond its defining keyword and an
// optional @index (JSON-LD 1.1 Section 9, "list object" and "set object").
auto validate_list_or_set(const JSON &value, const JSON::StringView keyword,
                          const JSON::Object::hash_type keyword_hash,
                          Pending &pending) -> bool {
  for (const auto &entry : value.as_object()) {
    if (entry.key_equals(keyword, keyword_hash)) {
      if (!entry.second.is_array()) {
        return false;
      }
      for (const auto &item : entry.second.as_array()) {
        pending.emplace_back(&item, Expect::Item);
      }
    } else if (entry.key_equals(KEYWORD_INDEX, KEYWORD_INDEX_HASH)) {
      if (!entry.second.is_string()) {
        return false;
      }
    } else {
      return false;
    }
  }
  return true;
}

// Validate one property array item, dispatching on its kind.
auto validate_item(const JSON &value, Pending &pending) -> bool {
  if (!value.is_object()) {
    return false;
  }
  if (value.defines(KEYWORD_VALUE, KEYWORD_VALUE_HASH)) {
    return is_value_object(value);
  }
  if (value.defines(KEYWORD_LIST, KEYWORD_LIST_HASH)) {
    return validate_list_or_set(value, KEYWORD_LIST, KEYWORD_LIST_HASH,
                                pending);
  }
  if (value.defines(KEYWORD_SET, KEYWORD_SET_HASH)) {
    return validate_list_or_set(value, KEYWORD_SET, KEYWORD_SET_HASH, pending);
  }
  return validate_node(value, pending);
}

} // namespace

auto jsonld_is_expanded(const JSON &document) -> bool {
  if (!document.is_array()) {
    return false;
  }

  Pending pending;
  for (const auto &item : document.as_array()) {
    pending.emplace_back(&item, Expect::Node);
  }

  while (!pending.empty()) {
    const auto [value, expect] = pending.back();
    pending.pop_back();
    if (!(expect == Expect::Node ? validate_node(*value, pending)
                                 : validate_item(*value, pending))) {
      return false;
    }
  }

  return true;
}

} // namespace sourcemeta::core
