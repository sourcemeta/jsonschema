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
  HTTPStatus status;
  JSON::StringView type{"about:blank"};
  JSON::StringView title{};
  JSON::StringView detail{};
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
