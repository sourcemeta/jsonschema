#ifndef SOURCEMETA_CORE_MCP_H_
#define SOURCEMETA_CORE_MCP_H_

#ifndef SOURCEMETA_CORE_MCP_EXPORT
#include <sourcemeta/core/mcp_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonrpc.h>

#include <cstddef>  // std::size_t
#include <cstdint>  // std::int64_t, std::uint8_t
#include <optional> // std::optional, std::nullopt
#include <utility>  // std::unreachable

/// @defgroup mcp MCP
/// @brief Helpers for building Model Context Protocol (MCP) envelopes.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// ```

namespace sourcemeta::core {

/// @ingroup mcp
/// The supported MCP protocol revisions.
enum class MCPProtocolVersion : std::uint8_t {
  V_2025_03_26,
  V_2025_06_18,
  V_2025_11_25,
};

/// @ingroup mcp
/// Get the canonical wire-format string for an MCP protocol version. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_protocol_version_string(
///            sourcemeta::core::MCPProtocolVersion::V_2025_11_25) ==
///        "2025-11-25");
/// ```
constexpr auto
mcp_protocol_version_string(const MCPProtocolVersion version) noexcept
    -> JSON::StringView {
  switch (version) {
    case MCPProtocolVersion::V_2025_03_26:
      return "2025-03-26";
    case MCPProtocolVersion::V_2025_06_18:
      return "2025-06-18";
    case MCPProtocolVersion::V_2025_11_25:
      return "2025-11-25";
  }
  std::unreachable();
}

/// @ingroup mcp
/// The MCP method name for the `initialize` request.
constexpr JSON::StringView MCP_METHOD_INITIALIZE{"initialize"};

/// @ingroup mcp
/// The MCP method name for the `ping` request.
constexpr JSON::StringView MCP_METHOD_PING{"ping"};

/// @ingroup mcp
/// The MCP method name for the `tools/list` request.
constexpr JSON::StringView MCP_METHOD_TOOLS_LIST{"tools/list"};

/// @ingroup mcp
/// The MCP method name for the `tools/call` request.
constexpr JSON::StringView MCP_METHOD_TOOLS_CALL{"tools/call"};

/// @ingroup mcp
/// The MCP method name for the `resources/list` request.
constexpr JSON::StringView MCP_METHOD_RESOURCES_LIST{"resources/list"};

/// @ingroup mcp
/// The MCP method name for the `resources/read` request.
constexpr JSON::StringView MCP_METHOD_RESOURCES_READ{"resources/read"};

/// @ingroup mcp
/// The MCP method name for the `resources/templates/list` request.
constexpr JSON::StringView MCP_METHOD_RESOURCES_TEMPLATES_LIST{
    "resources/templates/list"};

/// @ingroup mcp
/// The MCP method name for the `notifications/initialized` notification.
constexpr JSON::StringView MCP_METHOD_NOTIFICATIONS_INITIALIZED{
    "notifications/initialized"};

/// @ingroup mcp
/// The MCP error code returned when a requested resource cannot be found.
constexpr std::int64_t MCP_CODE_RESOURCE_NOT_FOUND{-32002};

/// @ingroup mcp
/// The MCP error code indicating that the client must complete a URL
/// elicitation flow before retrying.
constexpr std::int64_t MCP_CODE_URL_ELICITATION_REQUIRED{-32042};

/// @ingroup mcp
/// Check whether the given method name corresponds to an MCP request method
/// (notifications excluded). For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_is_request_method("initialize"));
/// assert(!sourcemeta::core::mcp_is_request_method("notifications/initialized"));
/// ```
constexpr auto mcp_is_request_method(const JSON::StringView method) noexcept
    -> bool {
  return method == MCP_METHOD_INITIALIZE || method == MCP_METHOD_PING ||
         method == MCP_METHOD_TOOLS_LIST || method == MCP_METHOD_TOOLS_CALL ||
         method == MCP_METHOD_RESOURCES_LIST ||
         method == MCP_METHOD_RESOURCES_READ ||
         method == MCP_METHOD_RESOURCES_TEMPLATES_LIST;
}

/// @ingroup mcp
/// Resolve an `MCP-Protocol-Version` header value into a known protocol
/// version, or `std::nullopt` when the value is unrecognised. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const auto resolved{
///     sourcemeta::core::mcp_resolve_protocol_version("2025-11-25")};
/// assert(resolved.has_value());
/// assert(resolved.value() ==
///        sourcemeta::core::MCPProtocolVersion::V_2025_11_25);
/// ```
constexpr auto
mcp_resolve_protocol_version(const JSON::StringView header) noexcept
    -> std::optional<MCPProtocolVersion> {
  if (header.empty()) {
    // Per the MCP Streamable HTTP transport spec: if the server does not
    // receive an MCP-Protocol-Version header, and has no other way to identify
    // the version, the server SHOULD assume protocol version 2025-03-26.
    // https://modelcontextprotocol.io/specification/2025-06-18/basic/transports#protocol-version-header
    return MCPProtocolVersion::V_2025_03_26;
  }
  if (header == "2025-11-25") {
    return MCPProtocolVersion::V_2025_11_25;
  }
  if (header == "2025-06-18") {
    return MCPProtocolVersion::V_2025_06_18;
  }
  if (header == "2025-03-26") {
    return MCPProtocolVersion::V_2025_03_26;
  }
  return std::nullopt;
}

/// @ingroup mcp
/// Whether the given protocol version supports per-tool `outputSchema`.
constexpr auto
mcp_supports_output_schema(const MCPProtocolVersion version) noexcept -> bool {
  return version != MCPProtocolVersion::V_2025_03_26;
}

/// @ingroup mcp
/// Whether the given protocol version supports `structuredContent` in tool
/// results.
constexpr auto
mcp_supports_structured_content(const MCPProtocolVersion version) noexcept
    -> bool {
  return version != MCPProtocolVersion::V_2025_03_26;
}

/// @ingroup mcp
/// Whether the given protocol version supports `resource_link` content blocks.
constexpr auto
mcp_supports_resource_link_content(const MCPProtocolVersion version) noexcept
    -> bool {
  return version != MCPProtocolVersion::V_2025_03_26;
}

/// @ingroup mcp
/// Whether the given protocol version supports the `title` field on the
/// implementation info object.
constexpr auto
mcp_supports_implementation_title(const MCPProtocolVersion version) noexcept
    -> bool {
  return version != MCPProtocolVersion::V_2025_03_26;
}

/// @ingroup mcp
/// Whether the given protocol version supports the `description` field on the
/// implementation info object.
constexpr auto mcp_supports_implementation_description(
    const MCPProtocolVersion version) noexcept -> bool {
  return version == MCPProtocolVersion::V_2025_11_25;
}

/// @ingroup mcp
/// Whether the given protocol version supports the `websiteUrl` field on the
/// implementation info object.
constexpr auto mcp_supports_implementation_website_url(
    const MCPProtocolVersion version) noexcept -> bool {
  return version == MCPProtocolVersion::V_2025_11_25;
}

/// @ingroup mcp
/// Whether the given protocol version supports JSON-RPC 2.0 batching.
constexpr auto
mcp_supports_jsonrpc_batching(const MCPProtocolVersion version) noexcept
    -> bool {
  return version == MCPProtocolVersion::V_2025_03_26;
}

/// @ingroup mcp
/// Build an MCP `text` content block carrying the given text payload. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const auto block{sourcemeta::core::mcp_make_text_block("hello")};
/// assert(block.at("type").to_string() == "text");
/// assert(block.at("text").to_string() == "hello");
/// ```
SOURCEMETA_CORE_MCP_EXPORT
auto mcp_make_text_block(const JSON::StringView text) -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build an MCP content block referencing a resource by URI. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const auto block{sourcemeta::core::mcp_make_resource_link(
///     sourcemeta::core::MCPProtocolVersion::V_2025_11_25, "file:///foo",
///     "text/plain")};
/// assert(block.at("type").to_string() == "resource_link");
/// ```
SOURCEMETA_CORE_MCP_EXPORT
auto mcp_make_resource_link(const MCPProtocolVersion version,
                            const JSON::StringView uri,
                            const JSON::StringView mime_type,
                            const JSON::StringView name = {},
                            const JSON::StringView description = {})
    -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build a JSON-RPC envelope wrapping a successful MCP tool call response from
/// the given result payload. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
/// #include <utility>
///
/// const auto identifier{sourcemeta::core::JSON{1}};
/// auto result{sourcemeta::core::JSON::make_object()};
/// result.assign("foo", sourcemeta::core::JSON{42});
/// const auto envelope{sourcemeta::core::mcp_make_tool_success(
///     sourcemeta::core::MCPProtocolVersion::V_2025_11_25, identifier,
///     std::move(result))};
/// assert(envelope.at("result").at("isError").to_boolean() == false);
/// ```
SOURCEMETA_CORE_MCP_EXPORT
auto mcp_make_tool_success(const MCPProtocolVersion version,
                           const sourcemeta::core::JSON &identifier,
                           sourcemeta::core::JSON result)
    -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build a JSON-RPC envelope wrapping a successful MCP tool call response from
/// caller-provided content blocks and a structured payload. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
/// #include <utility>
///
/// const auto identifier{sourcemeta::core::JSON{1}};
/// auto structured{sourcemeta::core::JSON::make_object()};
/// structured.assign("ok", sourcemeta::core::JSON{true});
/// auto blocks{sourcemeta::core::JSON::make_array()};
/// blocks.push_back(sourcemeta::core::mcp_make_text_block("done"));
/// const auto envelope{sourcemeta::core::mcp_make_tool_success(
///     sourcemeta::core::MCPProtocolVersion::V_2025_11_25, identifier,
///     std::move(structured), std::move(blocks))};
/// assert(envelope.at("result").at("isError").to_boolean() == false);
/// ```
SOURCEMETA_CORE_MCP_EXPORT
auto mcp_make_tool_success(const MCPProtocolVersion version,
                           const sourcemeta::core::JSON &identifier,
                           sourcemeta::core::JSON structured,
                           sourcemeta::core::JSON content_blocks)
    -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build a JSON-RPC envelope wrapping a failed MCP tool call response with the
/// given error message. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const auto identifier{sourcemeta::core::JSON{1}};
/// const auto envelope{
///     sourcemeta::core::mcp_make_tool_error(identifier, "Boom")};
/// assert(envelope.at("result").at("isError").to_boolean() == true);
/// ```
SOURCEMETA_CORE_MCP_EXPORT
auto mcp_make_tool_error(const sourcemeta::core::JSON &identifier,
                         const JSON::StringView message)
    -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build a JSON-RPC error envelope reporting that an MCP resource URI could
/// not be resolved. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const auto identifier{sourcemeta::core::JSON{3}};
/// const auto envelope{
///     sourcemeta::core::mcp_make_error_resource_not_found(identifier)};
/// assert(envelope.at("error").at("code").to_integer() == -32002);
/// ```
SOURCEMETA_CORE_MCP_EXPORT
auto mcp_make_error_resource_not_found(const sourcemeta::core::JSON &identifier)
    -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build an MCP resource descriptor as used in `resources/list` responses. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const auto resource{sourcemeta::core::mcp_make_resource(
///     "file:///a", "Alpha", "text/plain")};
/// assert(resource.at("uri").to_string() == "file:///a");
/// ```
SOURCEMETA_CORE_MCP_EXPORT
auto mcp_make_resource(const JSON::StringView uri, const JSON::StringView name,
                       const JSON::StringView mime_type,
                       const JSON::StringView description = {},
                       const std::optional<std::size_t> size = std::nullopt,
                       const std::optional<double> priority = std::nullopt)
    -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build an MCP `resources/read` content entry of `text` flavour. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const auto content{sourcemeta::core::mcp_make_resource_text_content(
///     "file:///a", "text/plain", "Hello")};
/// assert(content.at("text").to_string() == "Hello");
/// ```
SOURCEMETA_CORE_MCP_EXPORT
auto mcp_make_resource_text_content(const JSON::StringView uri,
                                    const JSON::StringView mime_type,
                                    const JSON::StringView text)
    -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Wrap a pre-built array of content entries into the MCP `resources/read`
/// result envelope. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
/// #include <utility>
///
/// auto contents{sourcemeta::core::JSON::make_array()};
/// contents.push_back(sourcemeta::core::mcp_make_resource_text_content(
///     "file:///a", "text/plain", "Hello"));
/// const auto result{
///     sourcemeta::core::mcp_make_resources_read_result(std::move(contents))};
/// assert(result.at("contents").size() == 1);
/// ```
SOURCEMETA_CORE_MCP_EXPORT
auto mcp_make_resources_read_result(sourcemeta::core::JSON contents)
    -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build a single entry for an MCP `resources/templates/list` response. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const auto entry{sourcemeta::core::mcp_make_resource_template(
///     "file:///{path}", "Files", "Resolves a file path", "text/plain")};
/// assert(entry.at("uriTemplate").to_string() == "file:///{path}");
/// ```
SOURCEMETA_CORE_MCP_EXPORT
auto mcp_make_resource_template(const JSON::StringView uri_template,
                                const JSON::StringView name,
                                const JSON::StringView description,
                                const JSON::StringView mime_type)
    -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Optional hints attached to an MCP tool descriptor.
struct MCPToolAnnotations {
  /// Optional human-readable title for the tool.
  JSON::StringView title = {};
  /// `true` when the tool guarantees no side effects.
  bool read_only = false;
  /// `true` when the tool may mutate or delete state.
  bool destructive = true;
  /// `true` when repeated invocations with the same input yield the same
  /// result.
  bool idempotent = false;
  /// `true` when the tool interacts with state outside the server's control.
  bool open_world = true;
};

/// @ingroup mcp
/// Build a single entry for an MCP `tools/list` response. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const auto entry{sourcemeta::core::mcp_make_tool_descriptor(
///     sourcemeta::core::MCPProtocolVersion::V_2025_11_25, "say", "Says hello",
///     sourcemeta::core::parse_json(R"({ "type": "object" })"))};
/// assert(entry.at("name").to_string() == "say");
/// ```
SOURCEMETA_CORE_MCP_EXPORT
auto mcp_make_tool_descriptor(
    const MCPProtocolVersion version, const JSON::StringView name,
    const JSON::StringView description, sourcemeta::core::JSON input_schema,
    std::optional<sourcemeta::core::JSON> output_schema = std::nullopt,
    const MCPToolAnnotations &annotations = {}) -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Implementation info advertised by an MCP server during the initialize
/// handshake.
struct MCPImplementation {
  /// Short machine-readable server name.
  JSON::StringView name;
  /// Semver-compatible server version.
  JSON::StringView version;
  /// Optional human-readable title.
  JSON::StringView title = {};
  /// Optional human-readable description.
  JSON::StringView description = {};
  /// Optional public website URL.
  JSON::StringView website_url = {};
};

/// @ingroup mcp
/// Boolean toggles for the MCP `capabilities` object returned during the
/// initialize handshake.
struct MCPServerCapabilities {
  /// Whether the server advertises prompts.
  bool prompts = false;
  /// Whether the server advertises resources.
  bool resources = false;
  /// Whether the server advertises tools.
  bool tools = false;
  /// Whether the server advertises logging.
  bool logging = false;
  /// Whether the server advertises completions.
  bool completions = false;
};

/// @ingroup mcp
/// Build the JSON-RPC envelope returned in response to an MCP `initialize`
/// request. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const auto request{sourcemeta::core::parse_json(R"JSON({
///   "jsonrpc": "2.0",
///   "id": 1,
///   "method": "initialize",
///   "params": { "protocolVersion": "2025-11-25" }
/// })JSON")};
/// const sourcemeta::core::MCPServerCapabilities capabilities;
/// const sourcemeta::core::MCPImplementation server{"srv", "1.0.0"};
/// const auto envelope{sourcemeta::core::mcp_make_initialize_result(
///     request, capabilities, server)};
/// assert(envelope.at("result").at("protocolVersion").to_string() ==
///        "2025-11-25");
/// ```
SOURCEMETA_CORE_MCP_EXPORT
auto mcp_make_initialize_result(const sourcemeta::core::JSON &request,
                                const MCPServerCapabilities &capabilities,
                                const MCPImplementation &server,
                                const JSON::StringView instructions = {})
    -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Borrow the `arguments` object from a JSON-RPC `tools/call` envelope, or
/// return `nullptr` if no `arguments` object is present. The returned pointer
/// is valid for the lifetime of the input envelope. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const auto envelope{sourcemeta::core::parse_json(R"JSON({
///   "jsonrpc": "2.0",
///   "id": 1,
///   "method": "tools/call",
///   "params": { "name": "foo", "arguments": { "x": 1 } }
/// })JSON")};
/// const auto *arguments{sourcemeta::core::mcp_tool_call_arguments(envelope)};
/// assert(arguments != nullptr);
/// assert(arguments->at("x").to_integer() == 1);
/// ```
SOURCEMETA_CORE_MCP_EXPORT
auto mcp_tool_call_arguments(const sourcemeta::core::JSON &envelope)
    -> const sourcemeta::core::JSON *;

} // namespace sourcemeta::core

#endif
