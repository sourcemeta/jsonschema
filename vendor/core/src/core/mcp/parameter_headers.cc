#include <sourcemeta/core/mcp.h>

#include "validation.h"

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/http.h>
#include <sourcemeta/core/text.h>
#include <sourcemeta/core/unicode.h>

#include <set>    // std::set
#include <string> // std::string, std::to_string
#include <vector> // std::vector

namespace {
using sourcemeta::core::JSON;
using sourcemeta::core::MCPHeaderParameter;

auto value_at(const JSON &arguments, const MCPHeaderParameter &parameter)
    -> const JSON * {
  const JSON *value{&arguments};
  for (const auto name : parameter.path) {
    if (!value->is_object()) {
      return nullptr;
    }
    value = value->try_at(name);
    if (value == nullptr) {
      return nullptr;
    }
  }
  return value->is_null() ? nullptr : value;
}

// The specification constrains each mirrored parameter name, type and location
// separately, so each constraint reports itself
// https://modelcontextprotocol.io/specification/2026-07-28/basic/transports/streamable-http#schema-extension
auto require_parameters(const std::span<const MCPHeaderParameter> parameters)
    -> void {
  std::set<std::string> names;
  for (const auto &parameter : parameters) {
    std::string name{parameter.name};
    sourcemeta::core::to_lowercase(name);
    // A name must be non-empty and match HTTP field-name token syntax, which
    // also rules out control characters
    sourcemeta::core::internal::require(
        sourcemeta::core::http_is_token(parameter.name),
        "Parameter header names must be non-empty HTTP tokens");
    // Only a property statically reachable from the schema root may be
    // mirrored, which a chain of property names expresses
    sourcemeta::core::internal::require(
        !parameter.path.empty(),
        "Parameter header descriptors must name a property path");
    // Primitive types only, and a JSON number is explicitly not permitted
    sourcemeta::core::internal::require(
        parameter.type == JSON::Type::String ||
            parameter.type == JSON::Type::Integer ||
            parameter.type == JSON::Type::Boolean,
        "Parameter headers only mirror strings, integers and booleans");
    sourcemeta::core::internal::require(
        names.insert(std::move(name)).second,
        "Parameter header names must be case-insensitively unique");
  }
}

auto safe_parameter(const JSON &value, const MCPHeaderParameter &parameter)
    -> bool {
  if (parameter.type == JSON::Type::Integer) {
    if (value.is_integer()) {
      const auto integer{value.to_integer()};
      return integer >= INT64_C(-9007199254740991) &&
             integer <= INT64_C(9007199254740991);
    }
    static const JSON MIN_SAFE_INTEGER{INT64_C(-9007199254740991)};
    static const JSON MAX_SAFE_INTEGER{INT64_C(9007199254740991)};
    return value.is_integral() && value >= MIN_SAFE_INTEGER &&
           value <= MAX_SAFE_INTEGER;
  }
  return value.type() == parameter.type;
}

auto parameter_text(const JSON &value) -> std::string {
  if (value.is_string()) {
    return std::string{value.to_string()};
  }
  if (value.is_boolean()) {
    return value.to_boolean() ? "true" : "false";
  }
  return std::to_string(value.as_integer());
}

auto decode(const JSON::StringView value, std::string &storage)
    -> std::optional<JSON::StringView> {
  if (!sourcemeta::core::mcp_is_plain_header_value(value)) {
    return std::nullopt;
  }

  if (sourcemeta::core::mcp_is_encoded_header_value(value)) {
    auto decoded{sourcemeta::core::base64_decode(
        sourcemeta::core::mcp_header_sentinel_body(value))};
    if (!decoded || !sourcemeta::core::is_valid_utf8(*decoded)) {
      return std::nullopt;
    }

    storage = std::move(*decoded);
    return JSON::StringView{storage};
  }

  return value;
}
} // namespace

namespace sourcemeta::core {
auto mcp_make_parameter_headers(
    const std::span<const MCPHeaderParameter> parameters, const JSON &arguments)
    -> std::vector<std::pair<std::string, std::string>> {
  require_parameters(parameters);
  internal::require(arguments.is_object(), "Tool arguments must be an object");
  std::vector<std::pair<std::string, std::string>> result;
  result.reserve(parameters.size());
  for (const auto &parameter : parameters) {
    if (const auto *value{value_at(arguments, parameter)}; value) {
      internal::require(safe_parameter(*value, parameter),
                        "Invalid mirrored argument type or unsafe integer");
      result.emplace_back("Mcp-Param-" + std::string{parameter.name},
                          mcp_encode_header_value(parameter_text(*value)));
    }
  }
  return result;
}

auto mcp_validate_parameter_headers(
    const MCPProtocolVersion version, const JSON &identifier,
    const std::span<const MCPHeaderParameter> parameters, const JSON &arguments,
    const std::span<const std::pair<JSON::StringView, JSON::StringView>>
        headers) -> std::optional<JSON> {
  internal::require(version == MCPProtocolVersion::V_2026_07_28,
                    "Parameter headers require MCP 2026-07-28");
  internal::require(internal::valid_id(identifier),
                    "Invalid MCP request identifier");
  require_parameters(parameters);
  if (!arguments.is_object()) {
    return mcp_make_error(version, &identifier, JSONRPC_CODE_INVALID_PARAMS,
                          "Invalid tool arguments");
  }
  for (const auto &parameter : parameters) {
    const std::string name{"Mcp-Param-" + std::string{parameter.name}};

    std::optional<JSON::StringView> header;
    for (const auto &candidate : headers) {
      if (equals_ignore_case(candidate.first, name)) {
        if (header) {
          return mcp_make_error_header_mismatch(version, &identifier,
                                                "Duplicate parameter header");
        }
        header = candidate.second;
      }
    }
    const auto *value{value_at(arguments, parameter)};
    if (value == nullptr) {
      if (header) {
        return mcp_make_error_header_mismatch(version, &identifier, name,
                                              *header);
      }
      continue;
    }
    if (!safe_parameter(*value, parameter)) {
      return mcp_make_error(version, &identifier, JSONRPC_CODE_INVALID_PARAMS,
                            "Invalid mirrored argument");
    }
    std::string decoded_storage;
    const auto decoded{header ? decode(*header, decoded_storage)
                              : std::nullopt};
    bool matches = false;
    if (decoded) {
      if (parameter.type == JSON::Type::Integer) {
        try {
          const auto number{parse_json(*decoded)};
          matches = number.is_number() && number == *value;
        } catch (const std::exception &) {
          matches = false;
        }
      } else {
        matches = value->is_string() ? *decoded == value->to_string()
                                     : *decoded == parameter_text(*value);
      }
    }
    if (!matches) {
      return mcp_make_error_header_mismatch(version, &identifier, name,
                                            header.value_or(""),
                                            parameter_text(*value));
    }
  }
  return std::nullopt;
}
} // namespace sourcemeta::core
