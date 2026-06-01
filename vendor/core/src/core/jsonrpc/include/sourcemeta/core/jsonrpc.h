#ifndef SOURCEMETA_CORE_JSONRPC_H_
#define SOURCEMETA_CORE_JSONRPC_H_

#ifndef SOURCEMETA_CORE_JSONRPC_EXPORT
#include <sourcemeta/core/jsonrpc_export.h>
#endif

#include <sourcemeta/core/json.h>

#include <cstdint>  // std::int64_t
#include <optional> // std::optional, std::nullopt

/// @defgroup jsonrpc JSON-RPC
/// @brief An implementation of the JSON-RPC 2.0 specification.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/jsonrpc.h>
/// ```

namespace sourcemeta::core {

/// @ingroup jsonrpc
/// The pre-defined JSON-RPC 2.0 error code for invalid JSON received by the
/// server.
constexpr std::int64_t JSONRPC_CODE_PARSE = -32700;

/// @ingroup jsonrpc
/// The pre-defined JSON-RPC 2.0 error code for a malformed request envelope.
constexpr std::int64_t JSONRPC_CODE_INVALID_REQUEST = -32600;

/// @ingroup jsonrpc
/// The pre-defined JSON-RPC 2.0 error code for unknown methods.
constexpr std::int64_t JSONRPC_CODE_METHOD_NOT_FOUND = -32601;

/// @ingroup jsonrpc
/// The pre-defined JSON-RPC 2.0 error code for invalid method parameters.
constexpr std::int64_t JSONRPC_CODE_INVALID_PARAMS = -32602;

/// @ingroup jsonrpc
/// The pre-defined JSON-RPC 2.0 error code for internal server errors.
constexpr std::int64_t JSONRPC_CODE_INTERNAL = -32603;

/// @ingroup jsonrpc
/// The lower bound of the JSON-RPC 2.0 reserved range for
/// implementation-defined server errors.
constexpr std::int64_t JSONRPC_CODE_SERVER_ERROR_MIN = -32099;

/// @ingroup jsonrpc
/// The upper bound of the JSON-RPC 2.0 reserved range for
/// implementation-defined server errors.
constexpr std::int64_t JSONRPC_CODE_SERVER_ERROR_MAX = -32000;

/// @ingroup jsonrpc
/// Check whether the given code lies within the JSON-RPC 2.0 reserved range
/// for implementation-defined server errors. For example:
///
/// ```cpp
/// #include <sourcemeta/core/jsonrpc.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::jsonrpc_is_server_error(-32050));
/// assert(!sourcemeta::core::jsonrpc_is_server_error(-32603));
/// ```
SOURCEMETA_CORE_JSONRPC_EXPORT
auto jsonrpc_is_server_error(const std::int64_t code) -> bool;

/// @ingroup jsonrpc
/// Check whether the given JSON value is a JSON-RPC 2.0 batch envelope. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonrpc.h>
/// #include <cassert>
///
/// const auto payload{sourcemeta::core::parse_json(R"([])")};
/// assert(sourcemeta::core::jsonrpc_is_batch(payload));
/// ```
SOURCEMETA_CORE_JSONRPC_EXPORT
auto jsonrpc_is_batch(const sourcemeta::core::JSON &payload) -> bool;

/// @ingroup jsonrpc
/// Check whether the given JSON value is a non-empty JSON-RPC 2.0 batch
/// envelope. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonrpc.h>
/// #include <cassert>
///
/// const auto payload{sourcemeta::core::parse_json(
///     R"([ { "jsonrpc": "2.0", "method": "ping" } ])")};
/// assert(sourcemeta::core::jsonrpc_is_valid_batch(payload));
/// ```
SOURCEMETA_CORE_JSONRPC_EXPORT
auto jsonrpc_is_valid_batch(const sourcemeta::core::JSON &payload) -> bool;

/// @ingroup jsonrpc
/// Extract the request identifier from a JSON-RPC 2.0 envelope. Returns a
/// pointer to the identifier (string, number, or null per the specification)
/// or `nullptr` when the field is missing or not one of those types. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonrpc.h>
/// #include <cassert>
///
/// const auto request{sourcemeta::core::parse_json(
///     R"({ "jsonrpc": "2.0", "id": 7, "method": "ping" })")};
/// const auto *identifier{sourcemeta::core::jsonrpc_request_id(request)};
/// assert(identifier != nullptr);
/// assert(identifier->to_integer() == 7);
/// ```
SOURCEMETA_CORE_JSONRPC_EXPORT
auto jsonrpc_request_id(const sourcemeta::core::JSON &request)
    -> const sourcemeta::core::JSON *;

/// @ingroup jsonrpc
/// Check whether the given JSON value is a well-formed JSON-RPC 2.0 request.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonrpc.h>
/// #include <cassert>
///
/// const auto request{sourcemeta::core::parse_json(
///     R"({ "jsonrpc": "2.0", "id": 1, "method": "ping" })")};
/// assert(sourcemeta::core::jsonrpc_is_request(request));
/// ```
SOURCEMETA_CORE_JSONRPC_EXPORT
auto jsonrpc_is_request(const sourcemeta::core::JSON &request) -> bool;

/// @ingroup jsonrpc
/// Extract the method name from a JSON-RPC 2.0 envelope, or an empty view
/// when the method field is missing or not a string. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonrpc.h>
/// #include <cassert>
///
/// const auto request{sourcemeta::core::parse_json(
///     R"({ "jsonrpc": "2.0", "id": 1, "method": "ping" })")};
/// assert(sourcemeta::core::jsonrpc_method(request) == "ping");
/// ```
SOURCEMETA_CORE_JSONRPC_EXPORT
auto jsonrpc_method(const sourcemeta::core::JSON &request) -> JSON::StringView;

/// @ingroup jsonrpc
/// Extract the params from a JSON-RPC 2.0 envelope, or `nullptr` when the
/// value is missing or not an object or array. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonrpc.h>
/// #include <cassert>
///
/// const auto request{sourcemeta::core::parse_json(R"JSON({
///   "jsonrpc": "2.0",
///   "id": 1,
///   "method": "subtract",
///   "params": [ 42, 23 ]
/// })JSON")};
/// const auto *parameters{sourcemeta::core::jsonrpc_params(request)};
/// assert(parameters != nullptr);
/// assert(parameters->is_array());
/// ```
SOURCEMETA_CORE_JSONRPC_EXPORT
auto jsonrpc_params(const sourcemeta::core::JSON &request)
    -> const sourcemeta::core::JSON *;

/// @ingroup jsonrpc
/// Check whether the given JSON value is a well-formed JSON-RPC 2.0
/// notification (a request without an identifier). For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonrpc.h>
/// #include <cassert>
///
/// const auto request{sourcemeta::core::parse_json(
///     R"({ "jsonrpc": "2.0", "method": "notifications/initialized" })")};
/// assert(sourcemeta::core::jsonrpc_is_notification(request));
/// ```
SOURCEMETA_CORE_JSONRPC_EXPORT
auto jsonrpc_is_notification(const sourcemeta::core::JSON &request) -> bool;

/// @ingroup jsonrpc
/// Construct a successful JSON-RPC 2.0 response envelope with the given
/// identifier and result. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonrpc.h>
/// #include <cassert>
/// #include <utility>
///
/// const auto identifier{sourcemeta::core::JSON{1}};
/// auto result{sourcemeta::core::JSON::make_object()};
/// result.assign("foo", sourcemeta::core::JSON{42});
/// const auto envelope{
///     sourcemeta::core::jsonrpc_make_success(identifier, std::move(result))};
/// assert(envelope.at("id").to_integer() == 1);
/// ```
SOURCEMETA_CORE_JSONRPC_EXPORT
auto jsonrpc_make_success(const sourcemeta::core::JSON &identifier,
                          sourcemeta::core::JSON result)
    -> sourcemeta::core::JSON;

/// @ingroup jsonrpc
/// Construct a successful JSON-RPC 2.0 response envelope with the given
/// identifier and an empty object result. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonrpc.h>
/// #include <cassert>
///
/// const auto identifier{sourcemeta::core::JSON{1}};
/// const auto envelope{
///     sourcemeta::core::jsonrpc_make_success_empty(identifier)};
/// assert(envelope.at("result").is_object());
/// assert(envelope.at("result").empty());
/// ```
SOURCEMETA_CORE_JSONRPC_EXPORT
auto jsonrpc_make_success_empty(const sourcemeta::core::JSON &identifier)
    -> sourcemeta::core::JSON;

/// @ingroup jsonrpc
/// Construct a JSON-RPC 2.0 error response envelope. Passing `nullptr` for the
/// identifier writes an `"id": null` member, as the specification requires
/// when the originating request could not be parsed. The optional data
/// carries implementation-specific extra information. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonrpc.h>
/// #include <cassert>
///
/// const auto identifier{sourcemeta::core::JSON{1}};
/// const auto envelope{sourcemeta::core::jsonrpc_make_error(
///     &identifier, -32000, "Server error")};
/// assert(envelope.at("error").at("code").to_integer() == -32000);
/// ```
SOURCEMETA_CORE_JSONRPC_EXPORT
auto jsonrpc_make_error(const sourcemeta::core::JSON *identifier,
                        const std::int64_t code, const JSON::StringView message,
                        std::optional<sourcemeta::core::JSON> data =
                            std::nullopt) -> sourcemeta::core::JSON;

/// @ingroup jsonrpc
/// Get the canonical JSON-RPC 2.0 parse-error envelope. The returned reference
/// is to a static instance that lives for the duration of the program. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/jsonrpc.h>
/// #include <cassert>
///
/// const auto &envelope{sourcemeta::core::jsonrpc_make_error_parse()};
/// assert(envelope.at("error").at("code").to_integer() == -32700);
/// ```
SOURCEMETA_CORE_JSONRPC_EXPORT
auto jsonrpc_make_error_parse() -> const sourcemeta::core::JSON &;

/// @ingroup jsonrpc
/// Construct a JSON-RPC 2.0 invalid-request error envelope, optionally
/// carrying the offending request identifier. For example:
///
/// ```cpp
/// #include <sourcemeta/core/jsonrpc.h>
/// #include <cassert>
///
/// const auto envelope{sourcemeta::core::jsonrpc_make_error_invalid_request()};
/// assert(envelope.at("error").at("code").to_integer() == -32600);
/// ```
SOURCEMETA_CORE_JSONRPC_EXPORT
auto jsonrpc_make_error_invalid_request(
    const sourcemeta::core::JSON *identifier = nullptr)
    -> sourcemeta::core::JSON;

/// @ingroup jsonrpc
/// Construct a JSON-RPC 2.0 method-not-found error envelope for the given
/// request identifier. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonrpc.h>
/// #include <cassert>
///
/// const auto identifier{sourcemeta::core::JSON{2}};
/// const auto envelope{
///     sourcemeta::core::jsonrpc_make_error_method_not_found(identifier)};
/// assert(envelope.at("error").at("code").to_integer() == -32601);
/// ```
SOURCEMETA_CORE_JSONRPC_EXPORT
auto jsonrpc_make_error_method_not_found(
    const sourcemeta::core::JSON &identifier) -> sourcemeta::core::JSON;

/// @ingroup jsonrpc
/// Construct a JSON-RPC 2.0 invalid-params error envelope for the given
/// request identifier, optionally carrying implementation-specific data. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonrpc.h>
/// #include <cassert>
///
/// const auto identifier{sourcemeta::core::JSON{"req-7"}};
/// const auto envelope{
///     sourcemeta::core::jsonrpc_make_error_invalid_params(identifier)};
/// assert(envelope.at("error").at("code").to_integer() == -32602);
/// ```
SOURCEMETA_CORE_JSONRPC_EXPORT
auto jsonrpc_make_error_invalid_params(
    const sourcemeta::core::JSON &identifier,
    std::optional<sourcemeta::core::JSON> data = std::nullopt)
    -> sourcemeta::core::JSON;

/// @ingroup jsonrpc
/// Construct a JSON-RPC 2.0 internal-error envelope, optionally carrying the
/// offending request identifier. For example:
///
/// ```cpp
/// #include <sourcemeta/core/jsonrpc.h>
/// #include <cassert>
///
/// const auto envelope{sourcemeta::core::jsonrpc_make_error_internal()};
/// assert(envelope.at("error").at("code").to_integer() == -32603);
/// ```
SOURCEMETA_CORE_JSONRPC_EXPORT
auto jsonrpc_make_error_internal(const sourcemeta::core::JSON *identifier =
                                     nullptr) -> sourcemeta::core::JSON;

} // namespace sourcemeta::core

#endif
