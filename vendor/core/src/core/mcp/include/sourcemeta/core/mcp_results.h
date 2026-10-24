#ifndef SOURCEMETA_CORE_MCP_RESULTS_H_
#define SOURCEMETA_CORE_MCP_RESULTS_H_

#ifndef SOURCEMETA_CORE_MCP_EXPORT
#include <sourcemeta/core/mcp_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/mcp_capabilities.h>
#include <sourcemeta/core/mcp_headers.h>
#include <sourcemeta/core/mcp_protocol.h>

#include <cstddef>  // std::size_t
#include <cstdint>  // std::int64_t, std::uint8_t
#include <optional> // std::optional, std::nullopt
#include <ostream>  // std::ostream
#include <span>     // std::span
#include <utility>  // std::unreachable
#include <vector>   // std::vector

namespace sourcemeta::core {

/// @ingroup mcp
/// Cache scope for 2026-07-28 cacheable MCP results.
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/server/utilities/caching#cacheable-results
enum class MCPCacheScope : std::uint8_t {
  /// Public caching is permissible across multiple clients/users.
  Public,
  /// Result is private and must only be reused within the same authorization
  /// context.
  Private,
};

/// @ingroup mcp
/// Canonical string conversion for cache scope. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_cache_scope_string(
///            sourcemeta::core::MCPCacheScope::Public) == "public");
/// assert(sourcemeta::core::mcp_cache_scope_string(
///            sourcemeta::core::MCPCacheScope::Private) == "private");
/// ```
[[nodiscard]] constexpr auto
mcp_cache_scope_string(const MCPCacheScope scope) noexcept -> JSON::StringView {
  switch (scope) {
    case MCPCacheScope::Public:
      return "public";
    case MCPCacheScope::Private:
      return "private";
  }
    // Clang settles that no other value arrives, so only GCC and MSVC
    // need to be told that the end of this function is never reached
#if !defined(__clang__)
  std::unreachable();
#endif
}

/// @ingroup mcp
/// Parse a canonical cache scope string. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_resolve_cache_scope("public") ==
///        sourcemeta::core::MCPCacheScope::Public);
/// assert(sourcemeta::core::mcp_resolve_cache_scope("private") ==
///        sourcemeta::core::MCPCacheScope::Private);
/// assert(!sourcemeta::core::mcp_resolve_cache_scope("shared").has_value());
/// ```
[[nodiscard]] constexpr auto
mcp_resolve_cache_scope(const JSON::StringView scope) noexcept
    -> std::optional<MCPCacheScope> {
  if (scope == "public") {
    return MCPCacheScope::Public;
  }
  if (scope == "private") {
    return MCPCacheScope::Private;
  }
  return std::nullopt;
}

/// @ingroup mcp
/// Cache policy specifying TTL in milliseconds and scope for cacheable MCP
/// results. Both fields are required for cacheable operations in MCP
/// 2026-07-28.
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/server/utilities/caching#cacheable-results
struct MCPCachePolicy {
  /// Time-to-live in milliseconds. The schema bounds this below by zero, so a
  /// negative value is refused rather than folded to zero.
  ///
  /// @see
  /// https://modelcontextprotocol.io/specification/2026-07-28/server/utilities/caching#cacheable-results
  std::int64_t ttl_ms = 0;
  /// Cache scope (public or private).
  MCPCacheScope scope = MCPCacheScope::Private;
};

/// @ingroup mcp
/// Optional hints attached to an MCP tool descriptor. The defaults are the
/// conservative reading the specification assigns to a hint a server does not
/// state, so a descriptor built without them claims nothing favourable about
/// the tool.
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#toolannotations
struct MCPToolAnnotations {
  /// Optional human-readable title for the tool.
  JSON::StringView title = {};
  /// `true` as an advisory hint that the tool does not modify its environment.
  bool read_only = false;
  /// `true` when the tool may mutate or delete state.
  bool destructive = true;
  /// `true` when repeated invocations with the same arguments have no
  /// additional effect on the environment.
  bool idempotent = false;
  /// `true` when the tool interacts with state outside the server's control.
  bool open_world = true;
};

/// @ingroup mcp
/// Advisory resource/content annotations. String and audience views are
/// borrowed, so their referents must outlive any builder this is passed to.
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#annotations
struct MCPResourceAnnotations {
  /// Optional audience roles ("user" and/or "assistant"). Empty stays explicit.
  std::optional<std::span<const JSON::StringView>> audience = std::nullopt;
  /// Optional priority. The schema bounds this between zero and one, so a
  /// value outside them is refused rather than folded in.
  ///
  /// @see
  /// https://modelcontextprotocol.io/specification/2026-07-28/schema#annotations
  std::optional<double> priority = std::nullopt;
  /// Optional timestamp string. The caller supplies a valid ISO 8601 timestamp.
  JSON::StringView last_modified = {};
};

/// @ingroup mcp
/// Optional presentation fields shared by descriptors. All views borrow data.
/// A field the target revision does not define is dropped rather than
/// reported.
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#tool-title
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#tool-icons
struct MCPDescriptorPresentation {
  /// Human-readable title (2025-06-18 and later).
  JSON::StringView title = {};
  /// Optional icon array (2025-11-25 and later).
  const JSON *icons = nullptr;
};

/// @ingroup mcp
/// Serialize advisory annotations, checking audience roles and gating
/// lastModified to 2025-06-18 and later. Empty annotations remain an object.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <array>
/// #include <cassert>
///
/// constexpr std::array<sourcemeta::core::JSON::StringView, 2> ROLES{
///     {"user", "assistant"}};
/// const sourcemeta::core::MCPResourceAnnotations annotations{
///     .audience = ROLES, .priority = 0.5};
/// assert(sourcemeta::core::mcp_serialize_resource_annotations(
///            sourcemeta::core::MCPProtocolVersion::V_2025_03_26,
///            annotations) ==
///        sourcemeta::core::parse_json(R"JSON({
///          "audience": [ "user", "assistant" ],
///          "priority": 0.5
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#annotations
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_serialize_resource_annotations(MCPProtocolVersion version,
                                   const MCPResourceAnnotations &annotations)
    -> JSON;

/// @ingroup mcp
/// Build a base64 image or audio content block for an explicit revision.
/// Core checks the block shape, not the media bytes or MIME semantics.
/// Returns a bare content block for the caller to place in a result.
///
/// The encoding of the data is a schema format annotation rather than a stated
/// requirement, so it is not checked here, and a caller that cares about it
/// decodes the data itself. Contrast the address of a URL mode elicitation,
/// which the specification states as a requirement in prose and which is
/// therefore checked. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_make_image_block(
///            sourcemeta::core::MCPProtocolVersion::V_2025_03_26, "YWJj",
///            "image/png") ==
///        sourcemeta::core::parse_json(R"JSON({
///          "type": "image",
///          "data": "YWJj",
///          "mimeType": "image/png"
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#imagecontent
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_make_image_block(MCPProtocolVersion version, JSON::StringView data,
                     JSON::StringView mime_type,
                     const MCPResourceAnnotations &annotations = {}) -> JSON;
/// @ingroup mcp
/// Build a base64 audio block, with the same contracts as the image builder.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_make_audio_block(
///            sourcemeta::core::MCPProtocolVersion::V_2025_03_26, "YWJj",
///            "audio/wav") ==
///        sourcemeta::core::parse_json(R"JSON({
///          "type": "audio",
///          "data": "YWJj",
///          "mimeType": "audio/wav"
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#audiocontent
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_make_audio_block(MCPProtocolVersion version, JSON::StringView data,
                     JSON::StringView mime_type,
                     const MCPResourceAnnotations &annotations = {}) -> JSON;
/// @ingroup mcp
/// Embed a checked text/blob resource-content entry in a content block. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_make_embedded_resource(
///            sourcemeta::core::MCPProtocolVersion::V_2025_03_26,
///            sourcemeta::core::mcp_make_resource_blob_content(
///                "file:///a", "application/octet-stream", "YWJj")) ==
///        sourcemeta::core::parse_json(R"JSON({
///          "type": "resource",
///          "resource": {
///            "uri": "file:///a",
///            "mimeType": "application/octet-stream",
///            "blob": "YWJj"
///          }
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#embeddedresource
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_make_embedded_resource(MCPProtocolVersion version, JSON resource,
                           const MCPResourceAnnotations &annotations = {})
    -> JSON;
/// @ingroup mcp
/// Build a blob resource-content entry. The caller supplies base64-encoded
/// data. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_make_resource_blob_content(
///            "file:///a", "application/octet-stream", "YWJj") ==
///        sourcemeta::core::parse_json(R"JSON({
///          "uri": "file:///a",
///          "mimeType": "application/octet-stream",
///          "blob": "YWJj"
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#blobresourcecontents
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_make_resource_blob_content(JSON::StringView uri, JSON::StringView mime_type,
                               JSON::StringView blob) -> JSON;
/// @ingroup mcp
/// Build an MCP `text` content block carrying the given text payload. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_make_text_block("hello") ==
///        sourcemeta::core::parse_json(R"JSON({
///          "type": "text",
///          "text": "hello"
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#textcontent
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_make_text_block(const JSON::StringView text) -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build an MCP content block referencing a resource by URI. This block type
/// was introduced in 2025-06-18, so an earlier revision is rejected. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_make_resource_link(
///            sourcemeta::core::MCPProtocolVersion::V_2025_11_25,
///            "file:///foo", "My File", "text/plain", "A description") ==
///        sourcemeta::core::parse_json(R"JSON({
///          "type": "resource_link",
///          "uri": "file:///foo",
///          "name": "My File",
///          "description": "A description",
///          "mimeType": "text/plain"
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#resourcelink
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto mcp_make_resource_link(
    const MCPProtocolVersion version, const JSON::StringView uri,
    const JSON::StringView name, const JSON::StringView mime_type = {},
    const JSON::StringView description = {}) -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Decorate a successful MCP result object with version-appropriate envelope
/// fields (`resultType: "complete"` and optional
/// `_meta.io.modelcontextprotocol/serverInfo` for 2026-07-28). Takes and
/// returns a bare result object, so an earlier revision leaves it unchanged.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_decorate_result(
///            sourcemeta::core::MCPProtocolVersion::V_2025_11_25,
///            sourcemeta::core::parse_json(R"JSON({ "status": "ok" })JSON")) ==
///        sourcemeta::core::parse_json(R"JSON({ "status": "ok" })JSON"));
/// assert(sourcemeta::core::mcp_decorate_result(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28,
///            sourcemeta::core::parse_json(R"JSON({ "status": "ok" })JSON")) ==
///        sourcemeta::core::parse_json(R"JSON({
///          "status": "ok",
///          "resultType": "complete"
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#result
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto mcp_decorate_result(
    const MCPProtocolVersion version, sourcemeta::core::JSON result,
    const std::optional<MCPImplementation> &server_info = std::nullopt)
    -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Decorate an MCP result object with cache metadata (`ttlMs` and
/// `cacheScope`) for 2026-07-28. Takes and returns a bare result object. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_decorate_cacheable_result(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28,
///            sourcemeta::core::JSON::make_object(),
///            {.ttl_ms = 5000,
///             .scope = sourcemeta::core::MCPCacheScope::Private},
///            sourcemeta::core::MCP_METHOD_TOOLS_LIST) ==
///        sourcemeta::core::parse_json(R"JSON({
///          "resultType": "complete",
///          "ttlMs": 5000,
///          "cacheScope": "private"
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/server/utilities/caching#cacheable-results
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto mcp_decorate_cacheable_result(
    const MCPProtocolVersion version, sourcemeta::core::JSON result,
    const MCPCachePolicy &cache_policy, const JSON::StringView method)
    -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build an empty MCP response with `resultType: "complete"`.
///
/// The builders in this header come in two shapes, and the name says which.
/// A function named for a result returns the bare payload that belongs under
/// the `result` key, while one named for a response returns the whole
/// JSON-RPC envelope around it and therefore takes the identifier to echo.
/// That is the specification's own distinction, which pairs a result such as
/// `DiscoverResult` with the envelope `DiscoverResultResponse`, and reserves
/// `JSONRPCResultResponse` for the generic shape. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_make_empty_response(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28,
///            sourcemeta::core::JSON{0}) ==
///        sourcemeta::core::parse_json(R"JSON({
///          "jsonrpc": "2.0",
///          "id": 0,
///          "result": { "resultType": "complete" }
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#emptyresult
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#jsonrpcresultresponse
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto mcp_make_empty_response(
    const MCPProtocolVersion version, const sourcemeta::core::JSON &identifier,
    const std::optional<MCPImplementation> &server_info = std::nullopt)
    -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build a JSON-RPC envelope wrapping a successful MCP tool call response from
/// the given result payload. The payload is rendered as a text content block
/// and, from 2025-06-18 onward, also carried as structured content. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
/// #include <utility>
///
/// auto result{sourcemeta::core::JSON::make_object()};
/// result.assign("foo", sourcemeta::core::JSON{42});
/// assert(sourcemeta::core::mcp_make_tool_success(
///            sourcemeta::core::MCPProtocolVersion::V_2025_11_25,
///            sourcemeta::core::JSON{1}, std::move(result)) ==
///        sourcemeta::core::parse_json(R"JSON({
///          "jsonrpc": "2.0",
///          "id": 1,
///          "result": {
///            "content": [ { "type": "text", "text": "{\n  \"foo\": 42\n}" } ],
///            "structuredContent": { "foo": 42 },
///            "isError": false
///          }
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/server/tools
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto mcp_make_tool_success(
    const MCPProtocolVersion version, const sourcemeta::core::JSON &identifier,
    sourcemeta::core::JSON result,
    const std::optional<MCPImplementation> &server_info = std::nullopt)
    -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build a JSON-RPC envelope wrapping a successful MCP tool call response from
/// caller-provided content blocks and a structured payload. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
/// #include <utility>
///
/// auto structured{sourcemeta::core::JSON::make_object()};
/// structured.assign("ok", sourcemeta::core::JSON{true});
/// auto blocks{sourcemeta::core::JSON::make_array()};
/// blocks.push_back(sourcemeta::core::mcp_make_text_block("done"));
/// assert(sourcemeta::core::mcp_make_tool_success(
///            sourcemeta::core::MCPProtocolVersion::V_2025_11_25,
///            sourcemeta::core::JSON{1}, std::move(structured),
///            std::move(blocks)) ==
///        sourcemeta::core::parse_json(R"JSON({
///          "jsonrpc": "2.0",
///          "id": 1,
///          "result": {
///            "content": [ { "type": "text", "text": "done" } ],
///            "structuredContent": { "ok": true },
///            "isError": false
///          }
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/server/tools
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto mcp_make_tool_success(
    const MCPProtocolVersion version, const sourcemeta::core::JSON &identifier,
    sourcemeta::core::JSON structured, sourcemeta::core::JSON content_blocks,
    const std::optional<MCPImplementation> &server_info = std::nullopt)
    -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build a version-aware JSON-RPC envelope wrapping a failed MCP tool call
/// response with the given error message. For 2026-07-28, adds
/// `resultType: "complete"`. A tool failure is reported inside a successful
/// response rather than as a JSON-RPC error. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_make_tool_error(
///            sourcemeta::core::MCPProtocolVersion::V_2025_11_25,
///            sourcemeta::core::JSON{7}, "Schema not found") ==
///        sourcemeta::core::parse_json(R"JSON({
///          "jsonrpc": "2.0",
///          "id": 7,
///          "result": {
///            "content": [ { "type": "text", "text": "Schema not found" } ],
///            "isError": true
///          }
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/server/tools#error-handling
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto mcp_make_tool_error(
    const MCPProtocolVersion version, const sourcemeta::core::JSON &identifier,
    const JSON::StringView message,
    const std::optional<MCPImplementation> &server_info = std::nullopt)
    -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build an MCP resource descriptor as used in `resources/list` responses.
/// Returns a bare descriptor for the caller to collect into a list result.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_make_resource(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28, "file:///a",
///            "a", "text/plain") ==
///        sourcemeta::core::parse_json(R"JSON({
///          "uri": "file:///a",
///          "name": "a",
///          "mimeType": "text/plain"
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#resource
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_make_resource(const MCPProtocolVersion version, const JSON::StringView uri,
                  const JSON::StringView name, const JSON::StringView mime_type,
                  const JSON::StringView description = {},
                  const std::optional<std::size_t> size = std::nullopt,
                  const std::optional<double> priority = std::nullopt,
                  const MCPDescriptorPresentation &presentation = {},
                  const MCPResourceAnnotations &annotations = {})
    -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build an MCP `resources/read` content entry of `text` flavour. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_make_resource_text_content(
///            "file:///a", "text/plain", "Hello") ==
///        sourcemeta::core::parse_json(R"JSON({
///          "uri": "file:///a",
///          "mimeType": "text/plain",
///          "text": "Hello"
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#textresourcecontents
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto mcp_make_resource_text_content(
    const JSON::StringView uri, const JSON::StringView mime_type,
    const JSON::StringView text) -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Wrap a pre-built array of content entries into the MCP `resources/read`
/// result envelope with a required cache policy for 2026-07-28. Returns a bare
/// result object, so the caller still wraps it in a JSON-RPC envelope. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
/// #include <utility>
///
/// auto contents{sourcemeta::core::JSON::make_array()};
/// contents.push_back(sourcemeta::core::mcp_make_resource_text_content(
///     "file:///a", "text/plain", "Hello"));
/// assert(sourcemeta::core::mcp_make_resources_read_result(
///            sourcemeta::core::MCPProtocolVersion::V_2025_11_25,
///            std::move(contents),
///            {.ttl_ms = 30000,
///             .scope = sourcemeta::core::MCPCacheScope::Public}) ==
///        sourcemeta::core::parse_json(R"JSON({
///          "contents": [
///            { "uri": "file:///a", "mimeType": "text/plain", "text": "Hello" }
///          ]
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/server/resources
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto mcp_make_resources_read_result(
    const MCPProtocolVersion version, sourcemeta::core::JSON contents,
    const MCPCachePolicy &cache_policy) -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build a single entry for an MCP `resources/templates/list` response. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_make_resource_template(
///            sourcemeta::core::MCPProtocolVersion::V_2025_03_26,
///            "file:///{path}", "Files", "Resolves a file path",
///            "text/plain") ==
///        sourcemeta::core::parse_json(R"JSON({
///          "uriTemplate": "file:///{path}",
///          "name": "Files",
///          "description": "Resolves a file path",
///          "mimeType": "text/plain"
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#resourcetemplate
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto mcp_make_resource_template(
    const MCPProtocolVersion version, const JSON::StringView uri_template,
    const JSON::StringView name, const JSON::StringView description,
    const JSON::StringView mime_type,
    const MCPDescriptorPresentation &presentation = {},
    const MCPResourceAnnotations &annotations = {}) -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build a single entry for an MCP `tools/list` response.
/// Schemas must declare `type: object` at the root. The caller validates the
/// remaining schema content. The behavioural hints are always emitted, with
/// the conservative defaults the specification assigns to an absent hint. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_make_tool_descriptor(
///            sourcemeta::core::MCPProtocolVersion::V_2025_11_25, "say",
///            "Says hello",
///            sourcemeta::core::parse_json(
///                R"JSON({ "type": "object" })JSON")) ==
///        sourcemeta::core::parse_json(R"JSON({
///          "name": "say",
///          "description": "Says hello",
///          "inputSchema": { "type": "object" },
///          "annotations": {
///            "readOnlyHint": false,
///            "destructiveHint": true,
///            "idempotentHint": false,
///            "openWorldHint": true
///          }
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#tool
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto mcp_make_tool_descriptor(
    const MCPProtocolVersion version, const JSON::StringView name,
    const JSON::StringView description, sourcemeta::core::JSON input_schema,
    std::optional<sourcemeta::core::JSON> output_schema = std::nullopt,
    const MCPToolAnnotations &annotations = {},
    const MCPDescriptorPresentation &presentation = {})
    -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build an MCP `tools/list` result object for 2026-07-28 with required cache
/// metadata. Returns a bare result object. An absent cursor omits the member,
/// while an explicit empty one is preserved. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// auto entries{sourcemeta::core::JSON::make_array()};
/// entries.push_back(sourcemeta::core::mcp_make_tool_descriptor(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28, "calc",
///     "Calculator",
///     sourcemeta::core::parse_json(R"JSON({ "type": "object" })JSON")));
/// assert(sourcemeta::core::mcp_make_tools_list_result(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28,
///            sourcemeta::core::JSON{entries}, "cursor-abc",
///            {.ttl_ms = 120000,
///             .scope = sourcemeta::core::MCPCacheScope::Public})
///            .at("nextCursor") == sourcemeta::core::JSON{"cursor-abc"});
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/server/utilities/caching#cacheable-results
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto mcp_make_tools_list_result(
    const MCPProtocolVersion version, sourcemeta::core::JSON tools,
    const std::optional<JSON::StringView> next_cursor,
    const MCPCachePolicy &cache_policy) -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build an MCP `resources/list` result object for 2026-07-28 with required
/// cache metadata. Returns a bare result object. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// auto entries{sourcemeta::core::JSON::make_array()};
/// entries.push_back(sourcemeta::core::mcp_make_resource(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28, "file:///a", "a",
///     "text/plain"));
/// assert(sourcemeta::core::mcp_make_resources_list_result(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28,
///            sourcemeta::core::JSON{entries}, "cursor-abc",
///            {.ttl_ms = 120000,
///             .scope = sourcemeta::core::MCPCacheScope::Public}) ==
///        sourcemeta::core::parse_json(R"JSON({
///          "resultType": "complete",
///          "resources": [
///            { "uri": "file:///a", "name": "a", "mimeType": "text/plain" }
///          ],
///          "nextCursor": "cursor-abc",
///          "ttlMs": 120000,
///          "cacheScope": "public"
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/server/utilities/caching#cacheable-results
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto mcp_make_resources_list_result(
    const MCPProtocolVersion version, sourcemeta::core::JSON resources,
    const std::optional<JSON::StringView> next_cursor,
    const MCPCachePolicy &cache_policy) -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build an MCP `resources/templates/list` result object for 2026-07-28 with
/// required cache metadata. Returns a bare result object. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const auto entries{sourcemeta::core::parse_json(
///     R"JSON([ { "uriTemplate": "file:///{path}", "name": "Files" } ])JSON")};
/// assert(sourcemeta::core::mcp_make_resource_templates_list_result(
///            sourcemeta::core::MCPProtocolVersion::V_2025_11_25,
///            sourcemeta::core::JSON{entries}, std::nullopt,
///            {.ttl_ms = 0,
///             .scope = sourcemeta::core::MCPCacheScope::Private}) ==
///        sourcemeta::core::parse_json(R"JSON({
///          "resourceTemplates": [
///            { "uriTemplate": "file:///{path}", "name": "Files" }
///          ]
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/server/utilities/caching#cacheable-results
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_make_resource_templates_list_result(
    const MCPProtocolVersion version, sourcemeta::core::JSON resource_templates,
    const std::optional<JSON::StringView> next_cursor,
    const MCPCachePolicy &cache_policy) -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build an MCP `prompts/list` result object for 2026-07-28 with required cache
/// metadata. Returns a bare result object. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const auto entries{sourcemeta::core::parse_json(
///     R"JSON([ { "name": "review", "description": "Review" } ])JSON")};
/// assert(sourcemeta::core::mcp_make_prompts_list_result(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28,
///            sourcemeta::core::JSON{entries}, "cursor-abc",
///            {.ttl_ms = 120000,
///             .scope = sourcemeta::core::MCPCacheScope::Public}) ==
///        sourcemeta::core::parse_json(R"JSON({
///          "resultType": "complete",
///          "prompts": [ { "name": "review", "description": "Review" } ],
///          "nextCursor": "cursor-abc",
///          "ttlMs": 120000,
///          "cacheScope": "public"
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/server/utilities/caching#cacheable-results
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto mcp_make_prompts_list_result(
    const MCPProtocolVersion version, sourcemeta::core::JSON prompts,
    const std::optional<JSON::StringView> next_cursor,
    const MCPCachePolicy &cache_policy) -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build a prompt result from checked role/content messages. Returns a bare
/// result object. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_make_prompts_get_result(
///            sourcemeta::core::MCPProtocolVersion::V_2025_03_26,
///            "description",
///            sourcemeta::core::parse_json(R"JSON([
///              {
///                "role": "user",
///                "content": { "type": "text", "text": "hello" }
///              }
///            ])JSON")) ==
///        sourcemeta::core::parse_json(R"JSON({
///          "description": "description",
///          "messages": [
///            {
///              "role": "user",
///              "content": { "type": "text", "text": "hello" }
///            }
///          ]
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/server/prompts
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_make_prompts_get_result(MCPProtocolVersion version,
                            JSON::StringView description, JSON messages)
    -> JSON;

/// @ingroup mcp
/// Build a completion result. Values must be strings and contain at most 100
/// entries. A supplied total cannot be smaller than the returned entry count.
/// Returns a bare result object. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_make_completion_result(
///            sourcemeta::core::MCPProtocolVersion::V_2025_03_26,
///            sourcemeta::core::JSON::make_array(), 0, false) ==
///        sourcemeta::core::parse_json(R"JSON({
///          "completion": { "values": [], "total": 0, "hasMore": false }
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#completeresult
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_make_completion_result(MCPProtocolVersion version, JSON values,
                           std::optional<std::int64_t> total,
                           std::optional<bool> has_more) -> JSON;

/// @ingroup mcp
/// Serialize a complete MCP response for one of the six cacheable operations.
/// The result is copied before decoration. The source remains unchanged.
/// A cache policy is mandatory for 2026-07-28. Pass `std::nullopt` for legacy
/// responses. A precomputed discovery result is held to the same rules as the
/// builder that produces one, so its advertised revisions must be non-empty,
/// must include the revision being served, and must each be one this library
/// models.
/// This writes JSON only. Stdio callers must append a newline themselves. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
/// #include <sstream>
///
/// auto result{sourcemeta::core::JSON::make_object()};
/// result.assign("resources", sourcemeta::core::JSON::make_array());
/// std::ostringstream stream;
/// sourcemeta::core::mcp_write_result(
///     stream, sourcemeta::core::MCPProtocolVersion::V_2026_07_28,
///     "resources/list", sourcemeta::core::JSON{1}, result,
///     sourcemeta::core::MCPCachePolicy{
///         .ttl_ms = 0, .scope = sourcemeta::core::MCPCacheScope::Private});
/// assert(sourcemeta::core::parse_json(stream.str()) ==
///        sourcemeta::core::parse_json(R"JSON({
///          "jsonrpc": "2.0",
///          "id": 1,
///          "result": {
///            "resources": [],
///            "resultType": "complete",
///            "ttlMs": 0,
///            "cacheScope": "private"
///          }
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/server/utilities/caching#cacheable-results
SOURCEMETA_CORE_MCP_EXPORT
auto mcp_write_result(
    std::ostream &stream, MCPProtocolVersion version, JSON::StringView method,
    const JSON &identifier, const JSON &result,
    std::optional<MCPCachePolicy> cache_policy,
    std::optional<MCPImplementation> server_info = std::nullopt) -> void;

/// @ingroup mcp
/// Serialize a complete MCP response for one of the six cacheable operations
/// using move semantics, decorating the provided result in-place.
/// A cache policy is mandatory for 2026-07-28. Pass `std::nullopt` for legacy
/// responses. A precomputed discovery result is held to the same rules as the
/// builder that produces one, so its advertised revisions must be non-empty,
/// must include the revision being served, and must each be one this library
/// models.
/// This writes JSON only. Stdio callers must append a newline themselves.
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/server/utilities/caching#cacheable-results
SOURCEMETA_CORE_MCP_EXPORT
auto mcp_write_result(
    std::ostream &stream, MCPProtocolVersion version, JSON::StringView method,
    const JSON &identifier, JSON &&result,
    std::optional<MCPCachePolicy> cache_policy,
    std::optional<MCPImplementation> server_info = std::nullopt) -> void;

/// @ingroup mcp
/// Build the JSON-RPC envelope returned in response to an MCP `initialize`
/// request. The revision is the one the client asked for when the server
/// supports it and it uses the handshake, and otherwise the highest supported
/// handshake revision, so an unrecognized request negotiates down rather than
/// failing. The advertised list must be non-empty, every revision in it must
/// be one this library models, and at least one must use the handshake. A
/// request that is not readable, or whose parameters are missing or malformed,
/// does yield an error envelope. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <array>
/// #include <cassert>
///
/// constexpr std::array<sourcemeta::core::JSON::StringView, 4> SUPPORTED{
///     {"2025-03-26", "2025-06-18", "2025-11-25", "2026-07-28"}};
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
///            SUPPORTED) ==
///        sourcemeta::core::parse_json(R"JSON({
///          "jsonrpc": "2.0",
///          "id": 1,
///          "result": {
///            "protocolVersion": "2025-11-25",
///            "capabilities": {},
///            "serverInfo": { "name": "srv", "version": "1.0.0" }
///          }
///        })JSON"));
///
/// const auto unknown{sourcemeta::core::parse_json(R"JSON({
///   "jsonrpc": "2.0",
///   "id": 1,
///   "method": "initialize",
///   "params": {
///     "protocolVersion": "9999-01-01",
///     "capabilities": {},
///     "clientInfo": { "name": "client", "version": "1" }
///   }
/// })JSON")};
/// assert(sourcemeta::core::mcp_make_initialize_response(
///            unknown, sourcemeta::core::MCPServerCapabilities{}, server,
///            SUPPORTED)
///            .at("result")
///            .at("protocolVersion") == sourcemeta::core::JSON{"2025-11-25"});
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-11-25/basic/lifecycle#initialization
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto mcp_make_initialize_response(
    const sourcemeta::core::JSON &request,
    const MCPServerCapabilities &capabilities, const MCPImplementation &server,
    const std::span<const JSON::StringView> supported_versions,
    const JSON::StringView instructions = {}) -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build the JSON-RPC response envelope for an MCP 2026-07-28 `server/discover`
/// request. This is the stateless replacement for the initialization
/// handshake, so it carries the supported revisions, the capabilities and the
/// cache metadata in one response. The advertised list must be non-empty, must
/// include the revision being served, and every revision in it must be one
/// this library models. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <array>
/// #include <cassert>
///
/// constexpr std::array<sourcemeta::core::JSON::StringView, 1> SUPPORTED{
///     {"2026-07-28"}};
/// const sourcemeta::core::MCPImplementation server{.name = "srv",
///                                                  .version = "2.0.0"};
/// const auto envelope{sourcemeta::core::mcp_make_server_discover_response(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28,
///     sourcemeta::core::JSON{1}, sourcemeta::core::MCPServerCapabilities{},
///     server, SUPPORTED, "Instructions here",
///     {.ttl_ms = 3600000,
///      .scope = sourcemeta::core::MCPCacheScope::Public})};
/// assert(envelope.at("result").at("resultType") ==
///        sourcemeta::core::JSON{"complete"});
/// assert(envelope.at("result").at("supportedVersions") ==
///        sourcemeta::core::parse_json(R"JSON([ "2026-07-28" ])JSON"));
/// assert(envelope.at("result").at("cacheScope") ==
///        sourcemeta::core::JSON{"public"});
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#discoverresult
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto mcp_make_server_discover_response(
    const MCPProtocolVersion version, const sourcemeta::core::JSON &identifier,
    const MCPServerCapabilities &capabilities, const MCPImplementation &server,
    const std::span<const JSON::StringView> supported_versions,
    const JSON::StringView instructions, const MCPCachePolicy &cache_policy)
    -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build an MCP Multi Round-Trip Result (`input_required`). At least one of
/// `input_requests` or `request_state` must be provided. Only `tools/call`,
/// `resources/read`, and `prompts/get` may carry this result. The client
/// capabilities are checked so a server does not ask for input the client
/// never advertised.
///
/// A nested request that sends the user to an address has that address checked
/// for being a URI with a scheme, which is the format the specification
/// annotates the field with and the reading its prose requirement supports.
/// What makes an address usable within a particular scheme is left alone: the
/// specification restricts the scheme not at all, and each scheme states its
/// own rules in its own document, so enforcing one scheme's rules and not the
/// others would be a policy this builder has no grounds to choose. A caller
/// that needs an address it can actually open checks that where it opens it.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
/// #include <utility>
///
/// auto input_requests{sourcemeta::core::parse_json(R"JSON({
///   "confirm": {
///     "method": "elicitation/create",
///     "params": {
///       "message": "Confirm delete?",
///       "requestedSchema": { "type": "object", "properties": {} }
///     }
///   }
/// })JSON")};
/// const auto envelope{sourcemeta::core::mcp_make_input_required_response(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28, "tools/call",
///     sourcemeta::core::JSON{50}, std::move(input_requests),
///     "opaque-state-token-123",
///     sourcemeta::core::MCPClientCapabilities{.elicitation = true,
///                                             .elicitation_form = true})};
/// assert(envelope.at("result").at("resultType") ==
///        sourcemeta::core::JSON{"input_required"});
/// assert(envelope.at("result").at("requestState") ==
///        sourcemeta::core::JSON{"opaque-state-token-123"});
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/patterns/mrtr#supported-requests
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto mcp_make_input_required_response(
    const MCPProtocolVersion version, const JSON::StringView method,
    const sourcemeta::core::JSON &identifier,
    std::optional<sourcemeta::core::JSON> input_requests,
    std::optional<JSON::StringView> request_state,
    const MCPClientCapabilities &client_capabilities,
    const std::optional<MCPImplementation> &server_info = std::nullopt)
    -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Access `inputResponses` from an MRTR continuation request envelope. Returns
/// a borrowed pointer into the envelope, or null when the member is absent or
/// malformed. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const auto continuation{sourcemeta::core::parse_json(R"JSON({
///   "jsonrpc": "2.0",
///   "id": 51,
///   "method": "tools/call",
///   "params": {
///     "name": "my-tool",
///     "requestState": "token-xyz",
///     "inputResponses": { "confirm": { "value": true } }
///   }
/// })JSON")};
/// assert(*sourcemeta::core::mcp_request_input_responses(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28,
///            continuation) ==
///        sourcemeta::core::parse_json(
///            R"JSON({ "confirm": { "value": true } })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/patterns/mrtr
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_request_input_responses(MCPProtocolVersion version,
                            const sourcemeta::core::JSON &envelope)
    -> const sourcemeta::core::JSON *;

/// @ingroup mcp
/// Access `requestState` token from an MRTR continuation request envelope. The
/// returned view borrows the envelope. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const auto continuation{sourcemeta::core::parse_json(R"JSON({
///   "jsonrpc": "2.0",
///   "id": 51,
///   "method": "tools/call",
///   "params": { "name": "my-tool", "requestState": "token-xyz" }
/// })JSON")};
/// assert(sourcemeta::core::mcp_request_state(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28,
///            continuation) == "token-xyz");
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/patterns/mrtr
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_request_state(MCPProtocolVersion version,
                  const sourcemeta::core::JSON &envelope)
    -> std::optional<JSON::StringView>;

/// @ingroup mcp
/// Opt-in filters for a 2026-07-28 subscription. Explicit false values are
/// preserved. An absent value is not requested. Resource URI views are
/// borrowed, so the parsed parameters must outlive the filter.
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/patterns/subscriptions
struct MCPSubscriptionFilter {
  /// Opt in to notifications that the tool list changed.
  std::optional<bool> tools_list_changed = std::nullopt;
  /// Opt in to notifications that the prompt list changed.
  std::optional<bool> prompts_list_changed = std::nullopt;
  /// Opt in to notifications that the resource list changed.
  std::optional<bool> resources_list_changed = std::nullopt;
  /// Borrowed resource URIs whose updates are requested. An absent list
  /// requests no subscriptions, while an explicit empty list stays explicit.
  std::optional<std::vector<JSON::StringView>> resource_subscriptions =
      std::nullopt;
};

/// @ingroup mcp
/// Parse the filter in checked 2026-07-28 listen parameters. String views
/// borrow the parameters, while the optional URI vector owns only those views,
/// so the parameters must outlive the filter. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const auto parameters{sourcemeta::core::parse_json(R"JSON({
///   "_meta": {
///     "io.modelcontextprotocol/protocolVersion": "2026-07-28",
///     "io.modelcontextprotocol/clientCapabilities": {}
///   },
///   "notifications": {
///     "toolsListChanged": true,
///     "promptsListChanged": false
///   }
/// })JSON")};
/// const auto filter{sourcemeta::core::mcp_parse_subscription_filter(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28, parameters)};
/// assert(filter->tools_list_changed == true);
/// assert(filter->prompts_list_changed == false);
/// assert(!filter->resources_list_changed.has_value());
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/patterns/subscriptions
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_parse_subscription_filter(MCPProtocolVersion version,
                              const JSON &parameters)
    -> std::optional<MCPSubscriptionFilter>;

/// @ingroup mcp
/// Restrict requested events and URIs to those the server supports. The result
/// is the subset the server agreed to honor, so an event only carries a value
/// when both sides opted into it and an unsupported event stays absent. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const sourcemeta::core::MCPSubscriptionFilter requested{
///     .tools_list_changed = true, .prompts_list_changed = true};
/// const sourcemeta::core::MCPSubscriptionFilter supported{
///     .tools_list_changed = true};
/// const auto agreed{sourcemeta::core::mcp_intersect_subscription_filters(
///     requested, supported)};
/// assert(agreed.tools_list_changed == true);
/// assert(!agreed.prompts_list_changed.has_value());
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/patterns/subscriptions
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_intersect_subscription_filters(const MCPSubscriptionFilter &requested,
                                   const MCPSubscriptionFilter &supported)
    -> MCPSubscriptionFilter;

/// @ingroup mcp
/// Extract a raw result discriminator without validating support. Absent
/// resultType means complete on incoming legacy results. Unknown string
/// discriminators remain available to extensions. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_result_type(sourcemeta::core::parse_json(
///            R"JSON({ "resultType": "example/extension" })JSON")) ==
///        "example/extension");
/// assert(sourcemeta::core::mcp_result_type(
///            sourcemeta::core::parse_json(R"JSON({})JSON")) == "complete");
/// assert(!sourcemeta::core::mcp_result_type(
///             sourcemeta::core::parse_json(
///                 R"JSON({ "resultType": 1 })JSON"))
///             .has_value());
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/index#result-responses
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_result_type(const JSON &result) -> std::optional<JSON::StringView>;

/// @ingroup mcp
/// Recognize base and explicitly negotiated extension result types. The
/// extension span is the caller's negotiated allowlist, not arbitrary input.
/// Missing discriminators mean complete. Input_required and extensions require
/// 2026-07-28. The returned view borrows the result (or a static base name).
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <array>
/// #include <cassert>
///
/// const std::array<sourcemeta::core::JSON::StringView, 1> EXTENSIONS{
///     {"org.example/custom"}};
/// const auto custom{sourcemeta::core::parse_json(
///     R"JSON({ "resultType": "org.example/custom" })JSON")};
/// assert(sourcemeta::core::mcp_resolve_result_type(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28, custom,
///            EXTENSIONS) == "org.example/custom");
/// assert(!sourcemeta::core::mcp_resolve_result_type(
///             sourcemeta::core::MCPProtocolVersion::V_2025_11_25, custom,
///             EXTENSIONS)
///             .has_value());
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/index#result-responses
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto mcp_resolve_result_type(
    MCPProtocolVersion version, const JSON &result,
    std::span<const JSON::StringView> supported_extension_types = {})
    -> std::optional<JSON::StringView>;

/// @ingroup mcp
/// Check continuation field shapes, exact state echo when expected, and known
/// response kinds against the prior input requests. Unknown response keys are
/// ignored. Core does not validate the user's form values against arbitrary
/// JSON Schema, protect state integrity, or enforce new request IDs/history.
/// This overload checks field shapes without matching a state token. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const auto parameters{sourcemeta::core::parse_json(R"JSON({
///   "_meta": {
///     "io.modelcontextprotocol/protocolVersion": "2026-07-28",
///     "io.modelcontextprotocol/clientCapabilities": {}
///   },
///   "requestState": "state",
///   "inputResponses": {
///     "form": { "action": "accept", "content": { "value": 1.25 } }
///   }
/// })JSON")};
/// const auto prior{sourcemeta::core::parse_json(R"JSON({
///   "form": {
///     "method": "elicitation/create",
///     "params": {
///       "mode": "form",
///       "message": "value",
///       "requestedSchema": { "type": "object", "properties": {} }
///     }
///   }
/// })JSON")};
/// assert(sourcemeta::core::mcp_validate_continuation(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28, parameters,
///     prior));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/patterns/mrtr#client-requirements-basic-workflow
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_validate_continuation(MCPProtocolVersion version, const JSON &parameters,
                          const JSON &input_requests) -> bool;

/// @ingroup mcp
/// Check continuation fields and the state token, where the expected state
/// selects one of three outcomes. Passing `std::nullopt` requires the token to
/// be absent, which is how a server that issued no state enforces that the
/// retry carries none. Passing an empty string requires a token that is
/// present and empty. Passing any other string requires the token to equal it
/// exactly. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const auto parameters{sourcemeta::core::parse_json(R"JSON({
///   "_meta": {
///     "io.modelcontextprotocol/protocolVersion": "2026-07-28",
///     "io.modelcontextprotocol/clientCapabilities": {}
///   },
///   "requestState": "state"
/// })JSON")};
/// const auto prior{sourcemeta::core::JSON::make_object()};
/// assert(sourcemeta::core::mcp_validate_continuation(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28, parameters, prior,
///     "state"));
/// assert(!sourcemeta::core::mcp_validate_continuation(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28, parameters, prior,
///     "other"));
/// assert(!sourcemeta::core::mcp_validate_continuation(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28, parameters, prior,
///     std::nullopt));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/patterns/mrtr
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_validate_continuation(MCPProtocolVersion version, const JSON &parameters,
                          const JSON &input_requests,
                          std::optional<JSON::StringView> expected_state)
    -> bool;

/// @ingroup mcp
/// Construct a checked notification. The direction and parameters are explicit.
/// 2026-07-28 logging/progress require the current validated request context.
/// Logging must meet the requested severity and cannot use a subscription ID.
/// The caller must route it on the response stream of that same request.
/// For 2026-07-28 server list/resource updates, subscription_id and the already
/// acknowledged filter are mandatory. A server cancellation must name that
/// subscription. Runtime code owns acknowledgment ordering, token activity,
/// increasing progress, authorization and stream lifetime. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
/// #include <optional>
///
/// assert(sourcemeta::core::mcp_make_notification(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28, false,
///            "notifications/cancelled",
///            sourcemeta::core::parse_json(
///                R"JSON({ "requestId": 0, "reason": "done" })JSON"),
///            std::nullopt) ==
///        sourcemeta::core::parse_json(R"JSON({
///          "jsonrpc": "2.0",
///          "method": "notifications/cancelled",
///          "params": { "requestId": 0, "reason": "done" }
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/patterns/subscriptions
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_make_notification(MCPProtocolVersion version, bool from_server,
                      JSON::StringView method, JSON parameters,
                      std::optional<JSON> subscription_id,
                      const MCPSubscriptionFilter *acknowledged = nullptr,
                      const MCPRequestMeta *request_context = nullptr) -> JSON;

/// @ingroup mcp
/// Build an acknowledgement notification for `subscriptions/listen` with a
/// string or integer subscription ID. The filter is restricted to the
/// intersection of requested and server-supported events and URIs. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// sourcemeta::core::MCPSubscriptionFilter filter;
/// filter.prompts_list_changed = true;
/// assert(sourcemeta::core::mcp_make_subscription_acknowledged_notification(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28,
///            sourcemeta::core::JSON{123}, filter, filter) ==
///        sourcemeta::core::parse_json(R"JSON({
///          "jsonrpc": "2.0",
///          "method": "notifications/subscriptions/acknowledged",
///          "params": {
///            "_meta": { "io.modelcontextprotocol/subscriptionId": 123 },
///            "notifications": { "promptsListChanged": true }
///          }
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/patterns/subscriptions
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_make_subscription_acknowledged_notification(
    const MCPProtocolVersion version,
    const sourcemeta::core::JSON &subscription_id,
    const MCPSubscriptionFilter &requested,
    const MCPSubscriptionFilter &supported) -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build an acknowledgement notification for `subscriptions/listen` with a
/// string subscription ID. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// sourcemeta::core::MCPSubscriptionFilter filter;
/// filter.tools_list_changed = true;
/// assert(sourcemeta::core::mcp_make_subscription_acknowledged_notification(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28, "sub-42",
///            filter, filter) ==
///        sourcemeta::core::parse_json(R"JSON({
///          "jsonrpc": "2.0",
///          "method": "notifications/subscriptions/acknowledged",
///          "params": {
///            "_meta": { "io.modelcontextprotocol/subscriptionId": "sub-42" },
///            "notifications": { "toolsListChanged": true }
///          }
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/patterns/subscriptions
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_make_subscription_acknowledged_notification(
    const MCPProtocolVersion version, const JSON::StringView subscription_id,
    const MCPSubscriptionFilter &requested,
    const MCPSubscriptionFilter &supported) -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build a graceful close response for `subscriptions/listen`. Returns a
/// complete JSON-RPC envelope. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_make_subscription_close_response(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28,
///            sourcemeta::core::JSON{9})
///            .at("id") == sourcemeta::core::JSON{9});
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/patterns/subscriptions
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_make_subscription_close_response(
    const MCPProtocolVersion version, const sourcemeta::core::JSON &identifier,
    const std::optional<MCPImplementation> &server_info = std::nullopt)
    -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Access `io.modelcontextprotocol/subscriptionId` from request or result
/// metadata. Returns a borrowed pointer into the envelope, or null when
/// absent. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const auto envelope{sourcemeta::core::parse_json(R"JSON({
///   "jsonrpc": "2.0",
///   "method": "notifications/tools/list_changed",
///   "params": {
///     "_meta": { "io.modelcontextprotocol/subscriptionId": "sub-1" }
///   }
/// })JSON")};
/// assert(*sourcemeta::core::mcp_request_subscription_id(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28, envelope) ==
///        sourcemeta::core::JSON{"sub-1"});
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/patterns/subscriptions
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_request_subscription_id(MCPProtocolVersion version,
                            const sourcemeta::core::JSON &envelope)
    -> const sourcemeta::core::JSON *;

/// @ingroup mcp
/// Borrow the `arguments` object from a JSON-RPC `tools/call` envelope.
/// Returns null when the member is absent or is not an object, so a tool that
/// takes no arguments is indistinguishable from a malformed one here. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const auto envelope{sourcemeta::core::parse_json(R"JSON({
///   "jsonrpc": "2.0",
///   "id": 1,
///   "method": "tools/call",
///   "params": { "name": "foo", "arguments": { "x": 1 } }
/// })JSON")};
/// assert(*sourcemeta::core::mcp_tool_call_arguments(envelope) ==
///        sourcemeta::core::parse_json(R"JSON({ "x": 1 })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/server/tools
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_tool_call_arguments(const sourcemeta::core::JSON &envelope)
    -> const sourcemeta::core::JSON *;

} // namespace sourcemeta::core

#endif
