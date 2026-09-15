#ifndef SOURCEMETA_CORE_OPENAPI_PATHS_H_
#define SOURCEMETA_CORE_OPENAPI_PATHS_H_

#include <sourcemeta/core/openapi.h>

#include "helpers.h"
#include "path_item.h"

#include <sourcemeta/core/text.h>
#include <sourcemeta/core/uri.h>

#include <cstddef>     // std::size_t
#include <set>         // std::set
#include <string_view> // std::string_view

namespace sourcemeta::core {

constexpr auto OPENAPI_HASH_PATHS{JSON::Object::hash("paths"sv)};
constexpr auto OPENAPI_HASH_WEBHOOKS{JSON::Object::hash("webhooks"sv)};

// Two paths are "identical" when they differ only in what their template
// expressions are called, so comparing them means dropping those names while
// leaving every other byte in place
inline auto openapi_path_shape(const JSON::StringView path) -> JSON::String {
  JSON::String result;
  result.reserve(path.size());
  std::size_t cursor{0};
  while (cursor < path.size()) {
    const auto open{path.find('{', cursor)};
    if (open == JSON::StringView::npos) {
      break;
    }

    // A brace that never closes opens no expression, so what follows it is
    // ordinary text. Dropping it instead would make two paths that differ only
    // after an unclosed brace compare as one, and 3.1 constrains neither
    const auto close{path.find('}', open)};
    if (close == JSON::StringView::npos) {
      break;
    }

    result.append(path.substr(cursor, open - cursor)).append("{}");
    cursor = close + 1;
  }

  result.append(path.substr(cursor));
  return result;
}

// OpenAPI Specification 3.2.1, Section 4.8.2 states the grammar 3.1 left
// unwritten:
//
//     path-template = "/" *( path-segment "/" ) [ path-segment ]
//     path-segment  = 1*( path-literal / template-expression )
//     path-literal  = 1*pchar
//     template-expression = "{" template-expression-param-name "}"
//     template-expression-param-name = 1*( %x00-7A / %x7C / %x7E-10FFFF )
//
// So a segment carries at least one character, an expression carries a name of
// at least one character, and every character outside an expression is a
// `pchar`. The name admits "every Unicode character except { and }", which of
// the bytes of one holds only of a brace, so it is read byte by byte like the
// rest
inline auto openapi_is_path_template(const JSON::StringView path) -> bool {
  if (!path.starts_with('/')) {
    return false;
  }

  std::size_t cursor{1};
  bool segment_is_empty{true};
  while (cursor < path.size()) {
    if (path[cursor] == '/') {
      if (segment_is_empty) {
        return false;
      }

      segment_is_empty = true;
      cursor += 1;
    } else if (path[cursor] == '{') {
      const auto close{path.find('}', cursor + 1)};
      if (close == JSON::StringView::npos || close == cursor + 1 ||
          path.substr(cursor + 1, close - cursor - 1).find('{') !=
              JSON::StringView::npos) {
        return false;
      }

      segment_is_empty = false;
      cursor = close + 1;
    } else if (path[cursor] == '%') {
      if (!is_percent_triplet(path, cursor)) {
        return false;
      }

      segment_is_empty = false;
      cursor += 3;
      // A percent sign is a path character too, and the branch above is what
      // holds it to introducing a triplet, so nothing reaches here with one
    } else if (URI::is_pchar(path[cursor])) {
      segment_is_empty = false;
      cursor += 1;
    } else {
      return false;
    }
  }

  return true;
}

// OpenAPI Specification 3.1.1, Section 4.8.8: "Holds the relative paths to the
// individual endpoints and their operations"
inline auto openapi_check_paths(const JSON &document, OpenAPIWalk &walk)
    -> void {
  const auto *paths{document.try_at("paths", OPENAPI_HASH_PATHS)};
  if (paths == nullptr) {
    return;
  }

  const Pointer base{"paths"};
  openapi_record(walk, base, OpenAPIObjectKind::Paths);
  openapi_expect_object(*paths, base, "The Paths Object must be an object");

  std::set<JSON::String> shapes;
  for (const auto &entry : paths->as_object()) {
    if (entry.first.starts_with(OPENAPI_EXTENSION_PREFIX)) {
      continue;
    }

    const auto location{openapi_child(base, entry.first)};

    // OpenAPI Specification 3.1.1, Section 4.8.8: "The field name MUST begin
    // with a forward slash (`/`)"
    if (!entry.first.starts_with('/')) {
      throw OpenAPIError{location,
                         "The Paths Object keys must begin with a slash"};
    }

    // 3.1 says nothing more about the shape of a path than that, while 3.2
    // writes out a grammar and forbids repeating an expression: "Each
    // template expression MUST NOT appear more than once in a single path
    // template". Both are new in 3.2, so a path 3.1 accepts is still accepted
    // when a document declares 3.1
    if (walk.version == OpenAPIVersion::OPENAPI_3_2) {
      if (!openapi_is_path_template(entry.first)) {
        throw OpenAPIError{location,
                           "The Paths Object keys must take the form of a "
                           "path template"};
      }

      std::set<JSON::StringView> expressions;
      for (const auto &expression : openapi_brace_expressions(entry.first)) {
        if (!expressions.insert(expression).second) {
          throw OpenAPIError{
              location,
              "A path template must not repeat a template expression"};
        }
      }
    }

    // OpenAPI Specification 3.1.1, Section 4.8.8: "Templated paths with the
    // same hierarchy but different templated names MUST NOT exist as they are
    // identical". The published meta-schema cannot state this, as no keyword
    // there compares one property name against another
    if (!shapes.insert(openapi_path_shape(entry.first)).second) {
      throw OpenAPIError{
          location, "The Paths Object keys must not repeat a templated path"};
    }

    openapi_check_path_item(entry.second, location, walk);

    // Section 4.3.3: "only the entry document's Paths Object contributes URLs
    // to the described API", and the document framed is that entry document,
    // so every path it writes is an endpoint
    walk.endpoints.push_back(
        {.kind = OpenAPIOperationKind::Path,
         .path = entry.first,
         .path_item = openapi_location_uri(walk.base, location)});
  }
}

// OpenAPI Specification 3.1.1, Section 4.8.1: "webhooks | Map[string, Path
// Item Object] | The incoming webhooks that MAY be received as part of this
// API". Its keys are names rather than paths, and unlike the Paths Object this
// map carries no extension carve-out, so a member named `x-` is a webhook
inline auto openapi_check_webhooks(const JSON &document, OpenAPIWalk &walk)
    -> void {
  const auto *webhooks{document.try_at("webhooks", OPENAPI_HASH_WEBHOOKS)};
  if (webhooks == nullptr) {
    return;
  }

  const Pointer base{"webhooks"};
  openapi_expect_object(*webhooks, base,
                        "The OpenAPI Description webhooks must be an object");

  for (const auto &entry : webhooks->as_object()) {
    const auto location{openapi_child(base, entry.first)};
    openapi_check_path_item(entry.second, location, walk);
    walk.endpoints.push_back(
        {.kind = OpenAPIOperationKind::Webhook,
         .path = entry.first,
         .path_item = openapi_location_uri(walk.base, location)});
  }
}

} // namespace sourcemeta::core

#endif
