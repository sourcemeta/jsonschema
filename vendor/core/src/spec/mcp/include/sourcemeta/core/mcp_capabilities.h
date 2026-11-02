#ifndef SOURCEMETA_CORE_MCP_CAPABILITIES_H_
#define SOURCEMETA_CORE_MCP_CAPABILITIES_H_

#ifndef SOURCEMETA_CORE_MCP_EXPORT
#include <sourcemeta/core/mcp_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/mcp_protocol.h>

#include <optional> // std::optional, std::nullopt

namespace sourcemeta::core {

/// @ingroup mcp
/// Information about an MCP implementation (client or server). Every string is
/// a borrowed view and the icons array is a borrowed pointer, so the referents
/// must outlive any builder this is passed to. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <array>
/// #include <cassert>
///
/// constexpr std::array<sourcemeta::core::JSON::StringView, 1> SUPPORTED{
///     {"2025-11-25"}};
/// const auto request{sourcemeta::core::parse_json(R"JSON({
///   "jsonrpc": "2.0",
///   "id": 1,
///   "method": "initialize",
///   "params": {
///     "protocolVersion": "2025-11-25",
///     "capabilities": {},
///     "clientInfo": { "name": "client", "version": "1" }
///   }
/// })JSON")};
/// const sourcemeta::core::MCPImplementation server{.name = "srv",
///                                                  .version = "1.0.0"};
/// assert(sourcemeta::core::mcp_make_initialize_response(
///            request, sourcemeta::core::MCPServerCapabilities{}, server,
///            SUPPORTED) == sourcemeta::core::parse_json(R"JSON({
///          "jsonrpc": "2.0",
///          "id": 1,
///          "result": {
///            "protocolVersion": "2025-11-25",
///            "capabilities": {},
///            "serverInfo": { "name": "srv", "version": "1.0.0" }
///          }
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#implementation
struct MCPImplementation {
  /// Implementation identifier name.
  JSON::StringView name;
  /// Implementation version (not required to be SemVer).
  JSON::StringView version;
  /// Optional human-readable title.
  JSON::StringView title = {};
  /// Optional human-readable description.
  JSON::StringView description = {};
  /// Optional public website URL.
  JSON::StringView website_url = {};
  /// Optional borrowed array of icons (2025-11-25 and later).
  const JSON *icons = nullptr;
};

/// @ingroup mcp
/// Capabilities advertised by an MCP client. The typed flags describe the
/// capabilities this library understands, while the snapshot retains whatever
/// else the peer sent so that a parse followed by a serialize does not discard
/// it.
///
/// Roots and sampling are deprecated as of protocol version `2026-07-28`,
/// which states that new implementations should not adopt them. They remain
/// representable here because the library still speaks the revisions that
/// define them.
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/deprecated
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/index#_meta
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#clientcapabilities
struct MCPClientCapabilities {
  /// Whether the client advertises roots.
  bool roots = false;
  /// Whether roots advertise `listChanged` (legacy revisions only).
  bool roots_list_changed = false;
  /// Whether the client advertises sampling.
  bool sampling = false;
  /// Whether sampling advertises context support. Declaring it is what permits
  /// a server to ask for context from the requesting server or from all of
  /// them. Those two request values were deprecated in `2025-11-25`, which
  /// asks a server to avoid them and not to use them at all unless a client
  /// declares this capability. The capability itself is not deprecated.
  bool sampling_context = false;
  /// Whether sampling advertises tools support.
  bool sampling_tools = false;
  /// Whether the client advertises elicitation.
  bool elicitation = false;
  /// Whether elicitation advertises form support.
  bool elicitation_form = false;
  /// Whether elicitation advertises url support.
  bool elicitation_url = false;
  /// Optional extensions map for client capabilities.
  std::optional<sourcemeta::core::JSON> extensions = std::nullopt;
  /// Optional experimental map for client capabilities.
  std::optional<sourcemeta::core::JSON> experimental = std::nullopt;
  /// Optional owned source preserving the parsed capability settings.
  /// Keeps extension settings, unknown capabilities and 2025-11-25 task data.
  /// Typed flags and optional maps override this snapshot when serializing. The
  /// per-request metadata view does not allocate or populate this snapshot.
  std::optional<JSON> source = std::nullopt;
};

/// @ingroup mcp
/// Capabilities advertised by an MCP server.
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#servercapabilities
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
  /// Whether tools advertise `listChanged`.
  bool tools_list_changed = false;
  /// Whether resources advertise `subscribe`.
  bool resources_subscribe = false;
  /// Whether resources advertise `listChanged`.
  bool resources_list_changed = false;
  /// Whether prompts advertise `listChanged`.
  bool prompts_list_changed = false;
  /// Optional extensions map for capabilities.
  std::optional<sourcemeta::core::JSON> extensions = std::nullopt;
  /// Optional experimental map for capabilities.
  std::optional<sourcemeta::core::JSON> experimental = std::nullopt;
  /// Owned source preserving unknown capabilities, settings and legacy tasks.
  /// Typed flags and optional maps override their corresponding source fields.
  std::optional<JSON> source = std::nullopt;
};

/// @ingroup mcp
/// Parse client capabilities from a JSON object. A capability the given
/// revision does not define is ignored rather than reported, while a known
/// capability of the wrong type is rejected with @ref MCPError. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const auto capabilities{sourcemeta::core::mcp_parse_client_capabilities(
///     sourcemeta::core::MCPProtocolVersion::V_2025_11_25,
///     sourcemeta::core::parse_json(R"JSON({
///       "roots": { "listChanged": true }
///     })JSON"))};
/// assert(capabilities.roots);
/// assert(capabilities.roots_list_changed);
/// assert(!capabilities.sampling);
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#clientcapabilities
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_parse_client_capabilities(const MCPProtocolVersion version,
                              const sourcemeta::core::JSON &capabilities)
    -> MCPClientCapabilities;

/// @ingroup mcp
/// Serialize client capabilities into a JSON object for the specified protocol
/// version. A capability the target revision does not define is dropped, so
/// serializing for an older revision than the one parsed is how a caller
/// downgrades an advertisement. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// sourcemeta::core::MCPClientCapabilities capabilities;
/// capabilities.roots = true;
/// capabilities.roots_list_changed = true;
/// assert(sourcemeta::core::mcp_serialize_client_capabilities(
///            sourcemeta::core::MCPProtocolVersion::V_2025_11_25,
///            capabilities) == sourcemeta::core::parse_json(R"JSON({
///          "roots": { "listChanged": true }
///        })JSON"));
/// assert(sourcemeta::core::mcp_serialize_client_capabilities(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28,
///            capabilities) ==
///        sourcemeta::core::parse_json(R"JSON({ "roots": {} })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#clientcapabilities
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_serialize_client_capabilities(const MCPProtocolVersion version,
                                  const MCPClientCapabilities &capabilities)
    -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Parse validated server capabilities, owning the snapshot and opaque
/// settings. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const auto capabilities{sourcemeta::core::mcp_parse_server_capabilities(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28,
///     sourcemeta::core::parse_json(R"JSON({
///       "tools": { "listChanged": true },
///       "resources": { "subscribe": true }
///     })JSON"))};
/// assert(capabilities.tools);
/// assert(capabilities.tools_list_changed);
/// assert(capabilities.resources_subscribe);
/// assert(!capabilities.prompts);
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#servercapabilities
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_parse_server_capabilities(MCPProtocolVersion version,
                              const JSON &capabilities)
    -> MCPServerCapabilities;

/// @ingroup mcp
/// Serialize server capabilities for an explicit revision. Typed flags override
/// the source snapshot. Opaque settings survive only in retained capabilities.
/// Throws @ref MCPError for malformed known fields. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// sourcemeta::core::MCPServerCapabilities capabilities;
/// capabilities.tools = true;
/// capabilities.tools_list_changed = true;
/// assert(sourcemeta::core::mcp_serialize_server_capabilities(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28,
///            capabilities) == sourcemeta::core::parse_json(R"JSON({
///          "tools": { "listChanged": true }
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#servercapabilities
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_serialize_server_capabilities(MCPProtocolVersion version,
                                  const MCPServerCapabilities &capabilities)
    -> JSON;

} // namespace sourcemeta::core

#endif
