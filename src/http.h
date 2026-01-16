#ifndef SOURCEMETA_JSONSCHEMA_CLI_HTTP_H_
#define SOURCEMETA_JSONSCHEMA_CLI_HTTP_H_

#ifndef NOMINMAX
#define NOMINMAX
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnewline-eof"
#endif
#include <cpr/cpr.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/uri.h>
#include <sourcemeta/core/yaml.h>

#include "error.h"
#include "logger.h"

#include <cstdint>     // std::uint64_t
#include <functional>  // std::ref
#include <sstream>     // std::ostringstream
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::jsonschema {

inline auto is_http_url(std::string_view identifier) -> bool {
  const sourcemeta::core::URI uri{std::string{identifier}};
  const auto maybe_scheme{uri.scheme()};
  return maybe_scheme.has_value() &&
         (maybe_scheme.value() == "https" || maybe_scheme.value() == "http");
}

inline auto uri_path_without_query_or_fragment(std::string_view uri)
    -> std::string_view {
  const auto fragment = uri.find('#');
  if (fragment != std::string_view::npos) {
    uri = uri.substr(0, fragment);
  }

  const auto query = uri.find('?');
  if (query != std::string_view::npos) {
    uri = uri.substr(0, query);
  }

  return uri;
}

inline auto is_likely_yaml(std::string_view uri, const cpr::Header &headers)
    -> bool {
  const auto content_type_iterator = headers.find("content-type");
  if (content_type_iterator != headers.end()) {
    const auto &content_type = content_type_iterator->second;
    if (content_type.starts_with("text/yaml") ||
        content_type.starts_with("text/x-yaml") ||
        content_type.starts_with("application/yaml") ||
        content_type.starts_with("application/x-yaml")) {
      return true;
    }
  }

  const auto path = uri_path_without_query_or_fragment(uri);
  return path.ends_with(".yaml") || path.ends_with(".yml");
}

inline auto fetch_http_response(const sourcemeta::core::Options &options,
                                std::string_view uri) -> cpr::Response {
  if (!is_http_url(uri)) {
    throw std::runtime_error{"The input is not an HTTP URL"};
  }

  LOG_VERBOSE(options) << "Fetching over HTTP: " << uri << "\n";
  const cpr::Response response{
      cpr::Get(cpr::Url{std::string{uri}}, cpr::Redirect{true})};

  if (response.status_code < 200 || response.status_code >= 300) {
    throw RemoteSchemaFetchError{std::string{uri}, response.status_code};
  }

  return response;
}

struct HTTPParsedJSON {
  sourcemeta::core::JSON document;
  sourcemeta::core::PointerPositionTracker positions;
  bool yaml{false};
};

inline auto parse_http_yaml_or_json(const sourcemeta::core::Options &options,
                                    std::string_view uri) -> HTTPParsedJSON {
  const cpr::Response response{fetch_http_response(options, uri)};

  sourcemeta::core::PointerPositionTracker positions;
  if (is_likely_yaml(uri, response.header)) {
    try {
      return {sourcemeta::core::parse_yaml(response.text, std::ref(positions)),
              std::move(positions), true};
    } catch (const sourcemeta::core::YAMLParseError &error) {
      throw RemoteSchemaYAMLParseError{std::string{uri}, error.what()};
    }
  }

  try {
    return {sourcemeta::core::parse_json(response.text, std::ref(positions)),
            std::move(positions), false};
  } catch (const sourcemeta::core::JSONParseError &error) {
    throw RemoteSchemaJSONParseError{std::string{uri}, error.line(),
                                     error.column(), error.what()};
  }
}

inline auto fetch_http_schema(const sourcemeta::core::Options &options,
                              std::string_view uri) -> sourcemeta::core::JSON {
  auto official_result{sourcemeta::core::schema_resolver(uri)};
  if (official_result.has_value()) {
    return official_result.value();
  }

  if (!is_http_url(uri)) {
    throw std::runtime_error{"The schema URI is not an HTTP URL"};
  }

  return parse_http_yaml_or_json(options, uri).document;
}

} // namespace sourcemeta::jsonschema

#endif
