#ifndef SOURCEMETA_CORE_HTTP_PROBLEM_H_
#define SOURCEMETA_CORE_HTTP_PROBLEM_H_

#ifndef SOURCEMETA_CORE_HTTP_EXPORT
#include <sourcemeta/core/http_export.h>
#endif

#include <sourcemeta/core/http_status.h>
#include <sourcemeta/core/json.h>

namespace sourcemeta::core {

/// @ingroup http
/// Fields of an RFC 9457 §3.1 Problem Details object.
struct HTTPProblemDetails {
  /// The HTTP status code for this occurrence of the problem
  HTTPStatus status;
  /// The identifier for the problem type
  JSON::StringView type{"about:blank"};
  /// The short human-readable summary of the problem type
  JSON::StringView title{};
  /// The human-readable explanation specific to this occurrence
  JSON::StringView detail{};
  /// The identifier for this specific occurrence of the problem
  JSON::StringView instance{};
};

/// @ingroup http
/// Build an RFC 9457 §3.1 Problem Details JSON object. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// const auto body{sourcemeta::core::http_make_problem_details({
///     .status = sourcemeta::core::HTTP_STATUS_NOT_FOUND,
///     .type   = "https://example.com/probs/not-found",
///     .detail = "The requested resource does not exist."})};
///
/// assert(body.at("status").to_integer() == 404);
/// assert(body.at("title").to_string() == "Not Found");
/// ```
SOURCEMETA_CORE_HTTP_EXPORT
auto http_make_problem_details(const HTTPProblemDetails &problem)
    -> sourcemeta::core::JSON;

} // namespace sourcemeta::core

#endif
