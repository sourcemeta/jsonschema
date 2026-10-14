#ifndef SOURCEMETA_CORE_MCP_ERROR_H_
#define SOURCEMETA_CORE_MCP_ERROR_H_

#ifndef SOURCEMETA_CORE_MCP_EXPORT
#include <sourcemeta/core/mcp_export.h>
#endif

#include <sourcemeta/core/http.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonrpc.h>
#include <sourcemeta/core/mcp_protocol.h>

#include <cstdint>   // std::int64_t, std::uint8_t
#include <optional>  // std::optional, std::nullopt
#include <span>      // std::span
#include <stdexcept> // std::invalid_argument

namespace sourcemeta::core {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup mcp
/// An error that represents a violation of an MCP contract, such as building a
/// result for a revision that does not define it, or supplying a descriptor
/// the specification rejects. It derives from the standard invalid-argument
/// error so that distinguishing it from the one the JSON module raises for a
/// non-finite number is a matter of catching the more specific type first. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
/// #include <string>
///
/// try {
///   // Resource links arrived in 2025-06-18
///   sourcemeta::core::mcp_make_resource_link(
///       sourcemeta::core::MCPProtocolVersion::V_2025_03_26, "file:///a", "a");
///   assert(false);
/// } catch (const sourcemeta::core::MCPError &error) {
///   assert(error.what() ==
///          std::string{"Resource links require MCP 2025-06-18 or later"});
/// }
/// ```
class SOURCEMETA_CORE_MCP_EXPORT MCPError : public std::invalid_argument {
public:
  /// Construct an error with a descriptive message
  MCPError(const char *message) : std::invalid_argument{message} {}
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

/// @ingroup mcp
/// Context for errors with no readable MCP request identifier. The two
/// legacy revisions require an error response to echo a readable identifier,
/// which a transport-level rejection cannot always produce, so the context
/// selects whether that requirement is enforced.
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/transports/streamable-http
enum class MCPErrorContext : std::uint8_t {
  /// A normal request response. Older schemas require a readable identifier.
  Request,
  /// An HTTP transport rejection. Notifications use an id-less response;
  /// unreadable legacy requests use JSON-RPC's null-ID transport fallback,
  /// which cannot satisfy the 2025-03-26/2025-06-18 MCP error schemas.
  Transport,
};

/// @ingroup mcp
/// Construct a checked MCP error. Null, fractional and container identifiers
/// are treated as unreadable. For 2026-07-28, rejects retired and undefined
/// reserved codes and checks the required data for version/capability errors.
/// Every revision the version error advertises must be one this library
/// models. The caller remains responsible for choosing an error that describes
/// the actual failure. Header mismatch data is optional. Returns a complete
/// JSON-RPC error envelope rather than a bare error object. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const sourcemeta::core::JSON identifier{1};
/// assert(sourcemeta::core::mcp_make_error(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28, &identifier,
///            sourcemeta::core::MCP_CODE_UNSUPPORTED_PROTOCOL_VERSION,
///            "Unsupported protocol version",
///            sourcemeta::core::parse_json(R"JSON({
///              "requested": "1999-01-01",
///              "supported": []
///            })JSON")) == sourcemeta::core::parse_json(R"JSON({
///          "jsonrpc": "2.0",
///          "id": 1,
///          "error": {
///            "code": -32022,
///            "message": "Unsupported protocol version",
///            "data": { "requested": "1999-01-01", "supported": [] }
///          }
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/index#responses
SOURCEMETA_CORE_MCP_EXPORT
auto mcp_make_error(MCPProtocolVersion version, const JSON *identifier,
                    std::int64_t code, JSON::StringView message,
                    std::optional<JSON> data = std::nullopt,
                    MCPErrorContext context = MCPErrorContext::Request) -> JSON;

/// @ingroup mcp
/// The legacy MCP error code returned when a requested resource cannot be
/// found.
constexpr std::int64_t MCP_CODE_RESOURCE_NOT_FOUND{-32002};

/// @ingroup mcp
/// The MCP error code indicating that the client must complete a URL
/// elicitation flow before retrying. Defined in 2025-11-25 and retired in
/// 2026-07-28.
constexpr std::int64_t MCP_CODE_URL_ELICITATION_REQUIRED{-32042};

/// @ingroup mcp
/// The MCP error code indicating that a transport header disagreed with the
/// request body.
constexpr std::int64_t MCP_CODE_HEADER_MISMATCH{-32020};

/// @ingroup mcp
/// The MCP error code indicating that a required capability was not
/// declared in request metadata.
constexpr std::int64_t MCP_CODE_MISSING_REQUIRED_CLIENT_CAPABILITY{-32021};

/// @ingroup mcp
/// The MCP error code indicating that the requested protocol version is not
/// supported by the receiver.
constexpr std::int64_t MCP_CODE_UNSUPPORTED_PROTOCOL_VERSION{-32022};

/// @ingroup mcp
/// Map an MCP or JSON-RPC error code to the HTTP status the specification
/// mandates for it on the Streamable HTTP transport, or nothing where it
/// mandates none.
///
/// Only 2026-07-28 ties a status to an error code on its own, and only for
/// four of them: an unimplemented method answers `404`, while a header
/// mismatch, a missing required client capability and an unsupported protocol
/// version each answer `400`.
///
/// Invalid parameters is deliberately absent despite carrying a mandate of its
/// own, because the mandate is written for one cause of that code, a request
/// that omits a required metadata field, while the same code also answers a
/// resource that cannot be found and an argument that disagrees with its
/// mirrored header. Those have no mandated status, and a missing resource
/// would be poorly served by the one the metadata rule gives. A caller
/// rejecting a request for missing metadata therefore answers `400` on its own
/// authority, which it can do because it chose that rejection and this helper
/// cannot see which cause produced the code.
///
/// Every other code in that revision, and every code in the three earlier
/// revisions, has no mandated status: those revisions tie a status to a
/// transport condition such as a terminated session rather than to a JSON-RPC
/// error, which in that era travels on a `200`.
///
/// So an absent result is not a gap to paper over. It says the choice belongs
/// to the adapter, which is why this returns nothing instead of inventing a
/// status, and in particular why a resource that cannot be found no longer
/// reports the server as being at fault. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_error_code_to_http_status(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28,
///            sourcemeta::core::JSONRPC_CODE_METHOD_NOT_FOUND) ==
///        sourcemeta::core::HTTP_STATUS_NOT_FOUND);
/// assert(sourcemeta::core::mcp_error_code_to_http_status(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28,
///            sourcemeta::core::MCP_CODE_HEADER_MISMATCH) ==
///        sourcemeta::core::HTTP_STATUS_BAD_REQUEST);
/// assert(!sourcemeta::core::mcp_error_code_to_http_status(
///             sourcemeta::core::MCPProtocolVersion::V_2026_07_28,
///             sourcemeta::core::JSONRPC_CODE_INTERNAL)
///             .has_value());
/// assert(!sourcemeta::core::mcp_error_code_to_http_status(
///             sourcemeta::core::MCPProtocolVersion::V_2025_11_25,
///             sourcemeta::core::JSONRPC_CODE_METHOD_NOT_FOUND)
///             .has_value());
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/transports/streamable-http#server-validation
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/index#_meta
constexpr auto mcp_error_code_to_http_status(const MCPProtocolVersion version,
                                             const std::int64_t code) noexcept
    -> std::optional<HTTPStatus> {
  if (version != MCPProtocolVersion::V_2026_07_28) {
    return std::nullopt;
  }
  if (code == JSONRPC_CODE_METHOD_NOT_FOUND) {
    return HTTP_STATUS_NOT_FOUND;
  }
  if (code == MCP_CODE_HEADER_MISMATCH ||
      code == MCP_CODE_MISSING_REQUIRED_CLIENT_CAPABILITY ||
      code == MCP_CODE_UNSUPPORTED_PROTOCOL_VERSION) {
    return HTTP_STATUS_BAD_REQUEST;
  }
  return std::nullopt;
}

/// @ingroup mcp
/// Build a JSON-RPC error response for unsupported protocol version (-32022).
/// This error is defined for 2026-07-28 only, and the list of supported
/// revisions must not be empty. Every revision in it must be one this library
/// models, which is a condition on the caller rather than on the peer, so a
/// violation throws instead of becoming an error envelope. An absent identifier
/// omits the member rather than sending a null one. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <array>
/// #include <cassert>
///
/// const sourcemeta::core::JSON identifier{7};
/// constexpr std::array<sourcemeta::core::JSON::StringView, 2> SUPPORTED{
///     {"2025-11-25", "2026-07-28"}};
/// assert(sourcemeta::core::mcp_make_error_unsupported_protocol_version(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28, &identifier,
///            "1999-01-01", SUPPORTED) == sourcemeta::core::parse_json(R"JSON({
///          "jsonrpc": "2.0",
///          "id": 7,
///          "error": {
///            "code": -32022,
///            "message": "Unsupported protocol version",
///            "data": {
///              "supported": [ "2025-11-25", "2026-07-28" ],
///              "requested": "1999-01-01"
///            }
///          }
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/index#error-codes
SOURCEMETA_CORE_MCP_EXPORT
auto mcp_make_error_unsupported_protocol_version(
    const MCPProtocolVersion version, const sourcemeta::core::JSON *identifier,
    const JSON::StringView requested,
    const std::span<const JSON::StringView> supported)
    -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build a JSON-RPC error response for missing required client capability
/// (-32021). This error is defined for 2026-07-28 only, and the required
/// capabilities must themselves be a valid client capability object. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const sourcemeta::core::JSON identifier{"x"};
/// assert(sourcemeta::core::mcp_make_error_missing_required_capability(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28, &identifier,
///            sourcemeta::core::parse_json(R"JSON({ "elicitation": {} })JSON"),
///            "Elicitation is required") ==
///        sourcemeta::core::parse_json(R"JSON({
///          "jsonrpc": "2.0",
///          "id": "x",
///          "error": {
///            "code": -32021,
///            "message": "Elicitation is required",
///            "data": { "requiredCapabilities": { "elicitation": {} } }
///          }
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/index#error-codes
SOURCEMETA_CORE_MCP_EXPORT
auto mcp_make_error_missing_required_capability(
    const MCPProtocolVersion version, const sourcemeta::core::JSON *identifier,
    sourcemeta::core::JSON required_capabilities,
    const JSON::StringView message) -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build a JSON-RPC error response for header mismatch (-32020). This overload
/// carries a caller-supplied message and no data, for a mismatch that names no
/// single header. Defined for 2026-07-28 only. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const sourcemeta::core::JSON identifier{1};
/// assert(sourcemeta::core::mcp_make_error_header_mismatch(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28, &identifier,
///            "Duplicate routing header") ==
///        sourcemeta::core::parse_json(R"JSON({
///          "jsonrpc": "2.0",
///          "id": 1,
///          "error": { "code": -32020, "message": "Duplicate routing header" }
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/transports/streamable-http#schema-extension
SOURCEMETA_CORE_MCP_EXPORT
auto mcp_make_error_header_mismatch(const MCPProtocolVersion version,
                                    const sourcemeta::core::JSON *identifier,
                                    const JSON::StringView message)
    -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build a JSON-RPC error response for header mismatch (-32020) with detailed
/// header mismatch payload, naming the header along with the conflicting
/// transport and body values. Defined for 2026-07-28 only. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const sourcemeta::core::JSON identifier{1};
/// assert(sourcemeta::core::mcp_make_error_header_mismatch(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28, &identifier,
///            "Mcp-Name", "foo",
///            "bar") == sourcemeta::core::parse_json(R"JSON({
///          "jsonrpc": "2.0",
///          "id": 1,
///          "error": {
///            "code": -32020,
///            "message": "Header mismatch",
///            "data": {
///              "header": "Mcp-Name",
///              "headerValue": "foo",
///              "bodyValue": "bar"
///            }
///          }
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/transports/streamable-http#schema-extension
SOURCEMETA_CORE_MCP_EXPORT
auto mcp_make_error_header_mismatch(const MCPProtocolVersion version,
                                    const sourcemeta::core::JSON *identifier,
                                    const JSON::StringView header_name,
                                    const JSON::StringView header_value,
                                    const JSON::StringView body_value)
    -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build a JSON-RPC error response for header mismatch (-32020) reporting an
/// unexpected header supplied without a corresponding body value. Defined for
/// 2026-07-28 only. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const sourcemeta::core::JSON identifier{1};
/// assert(sourcemeta::core::mcp_make_error_header_mismatch(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28, &identifier,
///            "Mcp-Name", "foo") == sourcemeta::core::parse_json(R"JSON({
///          "jsonrpc": "2.0",
///          "id": 1,
///          "error": {
///            "code": -32020,
///            "message": "Header mismatch: unexpected header provided",
///            "data": { "header": "Mcp-Name", "headerValue": "foo" }
///          }
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/transports/streamable-http#schema-extension
SOURCEMETA_CORE_MCP_EXPORT
auto mcp_make_error_header_mismatch(const MCPProtocolVersion version,
                                    const sourcemeta::core::JSON *identifier,
                                    const JSON::StringView header_name,
                                    const JSON::StringView header_value)
    -> sourcemeta::core::JSON;

/// @ingroup mcp
/// Build a version-aware JSON-RPC error response reporting that a resource
/// was not found. Emits -32002 for legacy revisions, and -32602 (Invalid
/// Params) for 2026-07-28, which retired the dedicated code. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// const sourcemeta::core::JSON identifier{1};
/// assert(sourcemeta::core::mcp_make_error_resource_not_found(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28,
///            &identifier) == sourcemeta::core::parse_json(R"JSON({
///          "jsonrpc": "2.0",
///          "id": 1,
///          "error": { "code": -32602, "message": "Resource not found" }
///        })JSON"));
/// assert(sourcemeta::core::mcp_make_error_resource_not_found(
///            sourcemeta::core::MCPProtocolVersion::V_2025_11_25,
///            &identifier) == sourcemeta::core::parse_json(R"JSON({
///          "jsonrpc": "2.0",
///          "id": 1,
///          "error": { "code": -32002, "message": "Resource not found" }
///        })JSON"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/server/resources#error-handling
SOURCEMETA_CORE_MCP_EXPORT
auto mcp_make_error_resource_not_found(const MCPProtocolVersion version,
                                       const sourcemeta::core::JSON *identifier)
    -> sourcemeta::core::JSON;

} // namespace sourcemeta::core

#endif
