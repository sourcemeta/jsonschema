#ifndef SOURCEMETA_CORE_MCP_HEADERS_H_
#define SOURCEMETA_CORE_MCP_HEADERS_H_

#ifndef SOURCEMETA_CORE_MCP_EXPORT
#include <sourcemeta/core/mcp_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/mcp_capabilities.h>
#include <sourcemeta/core/mcp_error.h>
#include <sourcemeta/core/mcp_protocol.h>

#include <cstdint>  // std::uint8_t
#include <optional> // std::optional
#include <span>     // std::span
#include <string>   // std::string
#include <utility>  // std::pair
#include <vector>   // std::vector

namespace sourcemeta::core {

/// @ingroup mcp
/// Header name for the protocol version in Streamable HTTP transport.
constexpr JSON::StringView MCP_HEADER_PROTOCOL_VERSION{"MCP-Protocol-Version"};

/// @ingroup mcp
/// Header name for the MCP method in Streamable HTTP transport.
constexpr JSON::StringView MCP_HEADER_METHOD{"Mcp-Method"};

/// @ingroup mcp
/// Header name for the target name/URI in Streamable HTTP transport.
constexpr JSON::StringView MCP_HEADER_NAME{"Mcp-Name"};

/// @ingroup mcp
/// Validation status returned by @ref mcp_validate_request_meta.
enum class MCPRequestMetaStatus : std::uint8_t {
  /// The request metadata is fully valid.
  Valid,
  /// The input is not a valid MCP request envelope.
  InvalidEnvelope,
  /// Metadata contains an invalid key name or malformed tracing value.
  InvalidMetaKey,
  /// Known client capability fields have invalid shapes.
  InvalidClientCapabilities,
  /// Client websiteUrl must be a string.
  ClientInfoWebsiteUrlNotString,
  /// Implementation icons have invalid shapes.
  InvalidClientInfo,
  /// logLevel must be one of the eight specified logging levels.
  InvalidLogLevel,
  /// progressToken must be a string or integer.
  InvalidProgressToken,
  /// Request envelope lacks a `params` property.
  MissingParams,
  /// The `params` property is not a JSON object.
  ParamsNotObject,
  /// The `params` object lacks a `_meta` property.
  MissingMeta,
  /// The `_meta` property is not a JSON object.
  MetaNotObject,
  /// `_meta` lacks `io.modelcontextprotocol/protocolVersion`.
  MissingProtocolVersion,
  /// `io.modelcontextprotocol/protocolVersion` is not a string.
  ProtocolVersionNotString,
  /// `io.modelcontextprotocol/protocolVersion` is not a recognized version.
  UnsupportedProtocolVersion,
  /// `_meta` lacks `io.modelcontextprotocol/clientCapabilities`.
  MissingClientCapabilities,
  /// `io.modelcontextprotocol/clientCapabilities` is not a JSON object.
  ClientCapabilitiesNotObject,
  /// `io.modelcontextprotocol/clientInfo` is not a JSON object.
  ClientInfoNotObject,
  /// `io.modelcontextprotocol/clientInfo` lacks required `name` property.
  MissingClientInfoName,
  /// `name` property in `io.modelcontextprotocol/clientInfo` is not a string.
  ClientInfoNameNotString,
  /// `io.modelcontextprotocol/clientInfo` lacks required `version` property.
  MissingClientInfoVersion,
  /// `version` property in `io.modelcontextprotocol/clientInfo` is not a
  /// string.
  ClientInfoVersionNotString,
  /// `title` property in `io.modelcontextprotocol/clientInfo` is not a string.
  ClientInfoTitleNotString,
  /// `description` property in `io.modelcontextprotocol/clientInfo` is not a
  /// string.
  ClientInfoDescriptionNotString,
};

/// @ingroup mcp
/// Parsed representation of 2026-07-28 request metadata. The protocol revision
/// is a value, while the capabilities, implementation, log level, metadata
/// object and progress token all reference the validated request, so the
/// request must outlive this view.
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/index#_meta
struct MCPRequestMeta {
  /// The declared protocol revision.
  MCPProtocolVersion protocol_version;
  /// Borrowed capabilities. The source request must outlive this view.
  const JSON &client_capabilities;
  /// Optional borrowed implementation description.
  std::optional<MCPImplementation> client_info;
  /// Optional requested log level.
  std::optional<JSON::StringView> log_level;
  /// Borrowed metadata. Never null after successful validation.
  const JSON &meta_object;
  /// Optional borrowed progress token.
  const JSON *progress_token = nullptr;
};

/// @ingroup mcp
/// Validate and extract MCP 2026-07-28 request metadata from a request
/// envelope. Use @ref mcp_validate_request_parameters for parameters. The
/// metadata is only present when the status is
/// @ref MCPRequestMetaStatus::Valid, and it borrows from the envelope. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const auto envelope{sourcemeta::core::parse_json(R"JSON({
///   "jsonrpc": "2.0",
///   "id": 1,
///   "method": "tools/list",
///   "params": {
///     "_meta": {
///       "io.modelcontextprotocol/protocolVersion": "2026-07-28",
///       "io.modelcontextprotocol/clientCapabilities": {}
///     }
///   }
/// })JSON")};
/// const auto [status,
///             meta]{sourcemeta::core::mcp_validate_request_meta(envelope)};
/// assert(status == sourcemeta::core::MCPRequestMetaStatus::Valid);
/// assert(meta->protocol_version ==
///        sourcemeta::core::MCPProtocolVersion::V_2026_07_28);
/// assert(!meta->client_info.has_value());
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/index#_meta
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_validate_request_meta(const sourcemeta::core::JSON &envelope)
    -> std::pair<MCPRequestMetaStatus, std::optional<MCPRequestMeta>>;

/// @ingroup mcp
/// Validate metadata in a parameters object, without guessing envelope shape.
/// This is the entry point for a caller that has already located the
/// parameters, such as one dispatching a batch member. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const auto parameters{sourcemeta::core::parse_json(R"JSON({
///   "_meta": {
///     "io.modelcontextprotocol/protocolVersion": "2026-07-28",
///     "io.modelcontextprotocol/clientCapabilities": {}
///   }
/// })JSON")};
/// assert(sourcemeta::core::mcp_validate_request_parameters(parameters)
///            .first == sourcemeta::core::MCPRequestMetaStatus::Valid);
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/index#_meta
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_validate_request_parameters(const JSON &parameters)
    -> std::pair<MCPRequestMetaStatus, std::optional<MCPRequestMeta>>;

/// @ingroup mcp
/// Construct an appropriate JSON-RPC error response from an invalid
/// @ref MCPRequestMetaStatus. An unrecognized revision becomes the dedicated
/// unsupported-version error carrying the supported list, an envelope that is
/// not a readable request becomes an invalid-request error, and every
/// remaining failure becomes an invalid-params error naming the offending
/// field. Passing the status for valid metadata throws
/// @ref MCPError, and so does a supported list naming a revision this library
/// does not model, since that is a condition on the caller rather than on the
/// peer. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <array>
/// #include <cassert>
///
/// const sourcemeta::core::JSON identifier{1};
/// constexpr std::array<sourcemeta::core::JSON::StringView, 1> SUPPORTED{
///     {"2026-07-28"}};
/// assert(sourcemeta::core::mcp_make_error_request_meta(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28, &identifier,
///            sourcemeta::core::MCPRequestMetaStatus::MissingMeta, "",
///            SUPPORTED)
///            .at("error")
///            .at("code")
///            .to_integer() == sourcemeta::core::JSONRPC_CODE_INVALID_PARAMS);
/// assert(sourcemeta::core::mcp_make_error_request_meta(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28, &identifier,
///            sourcemeta::core::MCPRequestMetaStatus::InvalidEnvelope, "",
///            SUPPORTED)
///            .at("error")
///            .at("code")
///            .to_integer() == sourcemeta::core::JSONRPC_CODE_INVALID_REQUEST);
/// constexpr auto UNSUPPORTED{
///     sourcemeta::core::MCPRequestMetaStatus::UnsupportedProtocolVersion};
/// assert(sourcemeta::core::mcp_make_error_request_meta(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28, &identifier,
///            UNSUPPORTED, "1999-01-01", SUPPORTED)
///            .at("error")
///            .at("code")
///            .to_integer() ==
///        sourcemeta::core::MCP_CODE_UNSUPPORTED_PROTOCOL_VERSION);
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/index#error-codes
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto mcp_make_error_request_meta(
    const MCPProtocolVersion version, const sourcemeta::core::JSON *identifier,
    const MCPRequestMetaStatus status, const JSON::StringView requested,
    const std::span<const JSON::StringView> supported)
    -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Borrow the target tool name or resource URI from a JSON-RPC request body.
/// The returned view is valid only while the source string remains alive and
/// unchanged. A resource read is named by its URI rather than by a name, and a
/// request that names no target yields nothing. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const auto envelope{sourcemeta::core::parse_json(R"JSON({
///   "method": "tools/call",
///   "params": { "name": "weather" }
/// })JSON")};
/// assert(sourcemeta::core::mcp_request_name_from_body(envelope) == "weather");
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/transports/streamable-http#standard-request-headers
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_request_name_from_body(const sourcemeta::core::JSON &envelope)
    -> std::optional<JSON::StringView>;

/// @ingroup mcp
/// Validate Streamable HTTP transport header values against the parsed
/// JSON-RPC request body for the specified protocol version. Returns nothing
/// when the headers agree with the body, and a ready-to-send JSON-RPC error
/// envelope otherwise. Every revision in the supported list must be one this
/// library models, which is a condition on the caller rather than on the peer,
/// so a violation throws rather than producing an envelope. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <array>
/// #include <cassert>
///
/// constexpr std::array<sourcemeta::core::JSON::StringView, 1> SUPPORTED{
///     {"2026-07-28"}};
/// const auto envelope{sourcemeta::core::parse_json(R"JSON({
///   "jsonrpc": "2.0",
///   "id": 1,
///   "method": "tools/call",
///   "params": {
///     "name": "weather",
///     "_meta": {
///       "io.modelcontextprotocol/protocolVersion": "2026-07-28",
///       "io.modelcontextprotocol/clientCapabilities": {}
///     }
///   }
/// })JSON")};
/// assert(!sourcemeta::core::mcp_validate_request_headers(
///             sourcemeta::core::MCPProtocolVersion::V_2026_07_28,
///             "2026-07-28", "tools/call", "weather", envelope, SUPPORTED)
///             .has_value());
/// assert(sourcemeta::core::mcp_validate_request_headers(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28, "2026-07-28",
///            "tools/call", "other", envelope, SUPPORTED)
///            .has_value());
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/transports/streamable-http#server-validation
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto mcp_validate_request_headers(
    const MCPProtocolVersion version,
    const std::optional<JSON::StringView> &protocol_version_header,
    const std::optional<JSON::StringView> &method_header,
    const std::optional<JSON::StringView> &name_header,
    const sourcemeta::core::JSON &envelope,
    const std::span<const JSON::StringView> supported_versions)
    -> std::optional<sourcemeta::core::JSON>;

/// @ingroup mcp
/// Encode a Mcp-Name or Mcp-Param value using the MCP 2026-07-28 sentinel
/// encoding when the value cannot be represented literally. A value passes
/// through only when every byte is printable ASCII, it neither begins nor ends
/// with a space, and it would not be mistaken for the sentinel form.
/// Everything else is encoded. Throws @ref MCPError for input that
/// is not UTF-8. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_encode_header_value("weather") == "weather");
/// assert(sourcemeta::core::mcp_encode_header_value(" café ") ==
///        "=?base64?IGNhZsOpIA==?=");
/// assert(sourcemeta::core::mcp_encode_header_value(" padded ") ==
///        "=?base64?IHBhZGRlZCA=?=");
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/transports/streamable-http#value-encoding
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_encode_header_value(JSON::StringView value) -> std::string;

/// @ingroup mcp
/// Validate a borrowed HTTP header collection. Names are case insensitive.
/// Duplicate recognized singleton fields are rejected. Unknown fields stay
/// opaque. HTTP framing, Origin and session validation belong to the adapter.
/// Returns nothing on success and a JSON-RPC error envelope otherwise. Every
/// revision in the supported list must be one this library models, which is a
/// condition on the caller rather than on the peer, so a violation throws
/// rather than producing an envelope. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <array>
/// #include <cassert>
/// #include <utility>
///
/// constexpr std::array<sourcemeta::core::JSON::StringView, 1> SUPPORTED{
///     {"2026-07-28"}};
/// const auto envelope{sourcemeta::core::parse_json(R"JSON({
///   "jsonrpc": "2.0",
///   "id": 1,
///   "method": "tools/call",
///   "params": {
///     "name": "weather",
///     "_meta": {
///       "io.modelcontextprotocol/protocolVersion": "2026-07-28",
///       "io.modelcontextprotocol/clientCapabilities": {}
///     }
///   }
/// })JSON")};
/// const std::array<std::pair<sourcemeta::core::JSON::StringView,
///                            sourcemeta::core::JSON::StringView>,
///                  3>
///     headers{{{"MCP-Protocol-Version", "2026-07-28"},
///              {"Mcp-Method", "tools/call"},
///              {"Mcp-Name", "weather"}}};
/// assert(!sourcemeta::core::mcp_validate_request_headers(
///             sourcemeta::core::MCPProtocolVersion::V_2026_07_28, headers,
///             envelope, SUPPORTED)
///             .has_value());
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/transports/streamable-http#server-validation
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto mcp_validate_request_headers(
    MCPProtocolVersion version,
    std::span<const std::pair<JSON::StringView, JSON::StringView>> headers,
    const JSON &envelope, std::span<const JSON::StringView> supported_versions)
    -> std::optional<JSON>;

/// @ingroup mcp
/// A caller-provided parameter header descriptor. String views borrow the
/// name and property path supplied by the caller. The path names the chain of
/// properties reaching the mirrored value from the arguments root, and the
/// type is restricted to the three primitives the specification permits.
///
/// A server declares these mirrors as annotations inside a tool's input
/// schema, and deriving descriptors from that schema is the caller's job
/// rather than this module's, because it means interpreting a schema. In
/// particular the caller owns the rule that an annotation counts only where
/// the property is reachable from the schema root through a chain of property
/// keys alone, never through an array, composition, conditional or reference
/// keyword, and that a tool whose annotation sits anywhere else is rejected
/// and excluded from the tool listing.
///
/// Given descriptors, this module enforces the remaining constraints on them:
/// each name is a non-empty HTTP token, which also rules out control
/// characters, the names are case-insensitively unique, only the three
/// primitive types are mirrored, and an integer value stays within the range
/// a double can represent exactly.
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/transports/streamable-http#schema-extension
/// https://modelcontextprotocol.io/specification/2026-07-28/server/tools#x-mcp-header
struct MCPHeaderParameter {
  /// The suffix of Mcp-Param-{name}.
  JSON::StringView name;
  /// Instance property path, with no array or reference traversal.
  std::vector<JSON::StringView> path;
  /// One of String, Integer or Boolean.
  JSON::Type type;
};

/// @ingroup mcp
/// Produce mirrored parameter headers. Missing/null values are omitted.
/// Throws @ref MCPError on a type mismatch or an unsafe integer. A
/// value that cannot be represented literally is sentinel encoded. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <array>
/// #include <cassert>
/// #include <string>
/// #include <utility>
/// #include <vector>
///
/// const std::array<sourcemeta::core::MCPHeaderParameter, 3> descriptors{
///     {{.name = "Label",
///       .path = {"nested", "text"},
///       .type = sourcemeta::core::JSON::Type::String},
///      {.name = "Count",
///       .path = {"number"},
///       .type = sourcemeta::core::JSON::Type::Integer},
///      {.name = "Enabled",
///       .path = {"enabled"},
///       .type = sourcemeta::core::JSON::Type::Boolean}}};
/// const auto arguments{sourcemeta::core::parse_json(R"JSON({
///   "nested": { "text": " café " },
///   "number": 42,
///   "enabled": false
/// })JSON")};
/// const std::vector<std::pair<std::string, std::string>> expected{
///     {"Mcp-Param-Label", "=?base64?IGNhZsOpIA==?="},
///     {"Mcp-Param-Count", "42"},
///     {"Mcp-Param-Enabled", "false"}};
/// assert(sourcemeta::core::mcp_make_parameter_headers(descriptors,
///                                                     arguments) == expected);
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/transports/streamable-http#schema-extension
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto
mcp_make_parameter_headers(std::span<const MCPHeaderParameter> parameters,
                           const JSON &arguments)
    -> std::vector<std::pair<std::string, std::string>>;

/// @ingroup mcp
/// Compare annotated parameters with raw HTTP headers case-insensitively.
/// Validates required mirrors, encoding, duplicates and numeric equivalence.
/// Protocol version must be 2026-07-28. The caller supplies the descriptors.
/// Returns nothing when every mirror agrees, and a JSON-RPC error envelope
/// otherwise. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <array>
/// #include <cassert>
/// #include <utility>
///
/// const std::array<sourcemeta::core::MCPHeaderParameter, 1> descriptors{
///     {{.name = "Count",
///       .path = {"number"},
///       .type = sourcemeta::core::JSON::Type::Integer}}};
/// const auto arguments{
///     sourcemeta::core::parse_json(R"JSON({ "number": 42 })JSON")};
/// const std::array<std::pair<sourcemeta::core::JSON::StringView,
///                            sourcemeta::core::JSON::StringView>,
///                  1>
///     headers{{{"Mcp-Param-Count", "42"}}};
/// assert(!sourcemeta::core::mcp_validate_parameter_headers(
///             sourcemeta::core::MCPProtocolVersion::V_2026_07_28,
///             sourcemeta::core::JSON{0}, descriptors, arguments, headers)
///             .has_value());
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/transports/streamable-http#server-validation
[[nodiscard]] SOURCEMETA_CORE_MCP_EXPORT auto mcp_validate_parameter_headers(
    MCPProtocolVersion version, const JSON &identifier,
    std::span<const MCPHeaderParameter> parameters, const JSON &arguments,
    std::span<const std::pair<JSON::StringView, JSON::StringView>> headers)
    -> std::optional<JSON>;

} // namespace sourcemeta::core

#endif
