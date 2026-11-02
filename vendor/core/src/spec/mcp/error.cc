#include <sourcemeta/core/mcp.h>

#include "helpers.h"
#include "validation.h"

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonrpc.h>

#include <span>    // std::span
#include <utility> // std::move

namespace sourcemeta::core {

auto mcp_make_error(const MCPProtocolVersion version, const JSON *identifier,
                    const std::int64_t code, const JSON::StringView message,
                    std::optional<JSON> data, const MCPErrorContext context)
    -> JSON {
  internal::require(version != MCPProtocolVersion::V_2026_07_28 ||
                        (code != MCP_CODE_RESOURCE_NOT_FOUND &&
                         code != MCP_CODE_URL_ELICITATION_REQUIRED),
                    "Retired MCP error code");
  if (version == MCPProtocolVersion::V_2026_07_28) {
    internal::require(code < -32099 || code > -32020 ||
                          code == MCP_CODE_HEADER_MISMATCH ||
                          code == MCP_CODE_MISSING_REQUIRED_CLIENT_CAPABILITY ||
                          code == MCP_CODE_UNSUPPORTED_PROTOCOL_VERSION,
                      "Undefined reserved MCP error code");
    if (code == MCP_CODE_UNSUPPORTED_PROTOCOL_VERSION) {
      internal::require(data.has_value() && data->is_object(),
                        "Protocol version error requires data");
      const auto *requested{
          data->try_at("requested", sourcemeta::core::MCP_HASH_REQUESTED)};
      const auto *supported{
          data->try_at("supported", sourcemeta::core::MCP_HASH_SUPPORTED)};
      internal::require(requested != nullptr && requested->is_string() &&
                            supported != nullptr && supported->is_array(),
                        "Invalid protocol version error data");
      for (const auto &revision : supported->as_array()) {
        internal::require(revision.is_string(),
                          "Supported protocol versions must be strings");
        // The list is what a client picks its next attempt from, so it can
        // only name revisions this library speaks, which is the rule the
        // discovery and initialization builders already apply to the same
        // advertisement
        // https://modelcontextprotocol.io/specification/2026-07-28/basic/versioning
        internal::require(
            mcp_resolve_protocol_version(revision.to_string()).has_value(),
            "Unknown server-supported MCP version");
      }
    } else if (code == MCP_CODE_MISSING_REQUIRED_CLIENT_CAPABILITY) {
      internal::require(data.has_value() && data->is_object(),
                        "Capability error requires data");
      const auto *required{
          data->try_at("requiredCapabilities",
                       sourcemeta::core::MCP_HASH_REQUIRED_CAPABILITIES)};
      internal::require(required != nullptr && internal::valid_capabilities(
                                                   version, *required, true),
                        "Invalid required client capabilities");
    }
  }
  const auto *valid_identifier{
      (identifier != nullptr) && internal::valid_id(*identifier) ? identifier
                                                                 : nullptr};
  const bool requires_identifier{version == MCPProtocolVersion::V_2025_03_26 ||
                                 version == MCPProtocolVersion::V_2025_06_18};
  internal::require((valid_identifier != nullptr) || !requires_identifier ||
                        context == MCPErrorContext::Transport,
                    "This MCP revision requires a readable error identifier");
  auto result{
      jsonrpc_make_error(valid_identifier, code, message, std::move(data))};
  if ((valid_identifier == nullptr) &&
      (!requires_identifier || identifier == nullptr)) {
    result.erase("id", MCP_HASH_ID);
  }
  return result;
}

auto mcp_make_error_unsupported_protocol_version(
    const MCPProtocolVersion version, const sourcemeta::core::JSON *identifier,
    const JSON::StringView requested,
    const std::span<const JSON::StringView> supported)
    -> sourcemeta::core::JSON {
  internal::require(version == MCPProtocolVersion::V_2026_07_28,
                    "This protocol error is defined for MCP 2026-07-28");
  internal::require(!supported.empty(),
                    "Supported protocol versions must not be empty");
  auto data{sourcemeta::core::JSON::make_object()};
  auto supported_array{sourcemeta::core::JSON::make_array()};
  for (const auto version_string : supported) {
    supported_array.push_back(sourcemeta::core::JSON{version_string});
  }
  data.assign_assume_new("supported", std::move(supported_array),
                         MCP_HASH_SUPPORTED);
  data.assign_assume_new("requested", sourcemeta::core::JSON{requested},
                         MCP_HASH_REQUESTED);

  auto envelope{
      mcp_make_error(version, identifier, MCP_CODE_UNSUPPORTED_PROTOCOL_VERSION,
                     "Unsupported protocol version", std::move(data))};
  return envelope;
}

auto mcp_make_error_missing_required_capability(
    const MCPProtocolVersion version, const sourcemeta::core::JSON *identifier,
    sourcemeta::core::JSON required_capabilities,
    const JSON::StringView message) -> sourcemeta::core::JSON {
  internal::require(version == MCPProtocolVersion::V_2026_07_28,
                    "This protocol error is defined for MCP 2026-07-28");
  auto data{sourcemeta::core::JSON::make_object()};
  data.assign_assume_new("requiredCapabilities",
                         std::move(required_capabilities),
                         MCP_HASH_REQUIRED_CAPABILITIES);

  auto envelope{mcp_make_error(version, identifier,
                               MCP_CODE_MISSING_REQUIRED_CLIENT_CAPABILITY,
                               message, std::move(data))};
  return envelope;
}

auto mcp_make_error_header_mismatch(const MCPProtocolVersion version,
                                    const sourcemeta::core::JSON *identifier,
                                    const JSON::StringView message)
    -> sourcemeta::core::JSON {
  internal::require(version == MCPProtocolVersion::V_2026_07_28,
                    "This protocol error is defined for MCP 2026-07-28");
  auto envelope{
      mcp_make_error(version, identifier, MCP_CODE_HEADER_MISMATCH, message)};
  return envelope;
}

auto mcp_make_error_header_mismatch(const MCPProtocolVersion version,
                                    const sourcemeta::core::JSON *identifier,
                                    const JSON::StringView header_name,
                                    const JSON::StringView header_value,
                                    const JSON::StringView body_value)
    -> sourcemeta::core::JSON {
  internal::require(version == MCPProtocolVersion::V_2026_07_28,
                    "This protocol error is defined for MCP 2026-07-28");
  auto data{sourcemeta::core::JSON::make_object()};
  data.assign_assume_new("header", sourcemeta::core::JSON{header_name},
                         MCP_HASH_HEADER);
  data.assign_assume_new("headerValue", sourcemeta::core::JSON{header_value},
                         MCP_HASH_HEADER_VALUE);
  data.assign_assume_new("bodyValue", sourcemeta::core::JSON{body_value},
                         MCP_HASH_BODY_VALUE);

  auto envelope{mcp_make_error(version, identifier, MCP_CODE_HEADER_MISMATCH,
                               "Header mismatch", std::move(data))};
  return envelope;
}

auto mcp_make_error_header_mismatch(const MCPProtocolVersion version,
                                    const sourcemeta::core::JSON *identifier,
                                    const JSON::StringView header_name,
                                    const JSON::StringView header_value)
    -> sourcemeta::core::JSON {
  internal::require(version == MCPProtocolVersion::V_2026_07_28,
                    "This protocol error is defined for MCP 2026-07-28");
  auto data{sourcemeta::core::JSON::make_object()};
  data.assign_assume_new("header", sourcemeta::core::JSON{header_name},
                         MCP_HASH_HEADER);
  data.assign_assume_new("headerValue", sourcemeta::core::JSON{header_value},
                         MCP_HASH_HEADER_VALUE);

  auto envelope{mcp_make_error(version, identifier, MCP_CODE_HEADER_MISMATCH,
                               "Header mismatch: unexpected header provided",
                               std::move(data))};
  return envelope;
}

auto mcp_make_error_resource_not_found(const MCPProtocolVersion version,
                                       const sourcemeta::core::JSON *identifier)
    -> sourcemeta::core::JSON {
  auto envelope{
      version == MCPProtocolVersion::V_2026_07_28
          ? mcp_make_error(version, identifier, JSONRPC_CODE_INVALID_PARAMS,
                           "Resource not found")
          : mcp_make_error(version, identifier, MCP_CODE_RESOURCE_NOT_FOUND,
                           "Resource not found")};
  return envelope;
}

} // namespace sourcemeta::core
