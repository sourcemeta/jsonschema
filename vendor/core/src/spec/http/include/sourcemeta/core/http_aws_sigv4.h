#ifndef SOURCEMETA_CORE_HTTP_AWS_SIGV4_H_
#define SOURCEMETA_CORE_HTTP_AWS_SIGV4_H_

#ifndef SOURCEMETA_CORE_HTTP_EXPORT
#include <sourcemeta/core/http_export.h>
#endif

#include <array>       // std::array
#include <cstdint>     // std::uint8_t
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::pair

namespace sourcemeta::core {

/// @ingroup http
/// The credentials used to sign a request with AWS Signature Version 4. The
/// session token is left empty when using long-term credentials.
struct HTTPAWSCredentials {
  /// The access key identifier
  std::string_view access_key_id;
  /// The secret access key
  std::string_view secret_access_key;
  /// The temporary session token used with short-term credentials
  std::string_view session_token;
};

/// @ingroup http
/// Compute the AWS Signature Version 4 canonical request from a request method,
/// path, query, headers, and payload hash. Each path segment and query
/// parameter is decoded and re-encoded into canonical form, so the path and
/// query may be passed exactly as they appear in the request target. The path
/// is optionally normalised by collapsing sequential slashes and removing dot
/// segments, which every service except Amazon S3 requires. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
/// #include <string_view>
/// #include <utility>
/// #include <vector>
///
/// const std::vector<std::pair<std::string_view, std::string_view>> headers{
///     {"Host", "example.amazonaws.com"}, {"X-Amz-Date", "20150830T123600Z"}};
/// const auto canonical{sourcemeta::core::http_aws_sigv4_canonical_request(
///     "GET", "/", "", headers,
///     "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")};
/// assert(canonical.starts_with("GET\n/\n"));
/// ```
auto SOURCEMETA_CORE_HTTP_EXPORT http_aws_sigv4_canonical_request(
    const std::string_view method, const std::string_view path,
    const std::string_view query,
    const std::span<const std::pair<std::string_view, std::string_view>>
        headers,
    const std::string_view payload_hash, const bool normalize = true)
    -> std::string;

/// @ingroup http
/// Compute the AWS Signature Version 4 signed headers list, the lowercased
/// header names sorted and joined with a semicolon. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
/// #include <string_view>
/// #include <utility>
/// #include <vector>
///
/// const std::vector<std::pair<std::string_view, std::string_view>> headers{
///     {"Host", "example.amazonaws.com"}, {"X-Amz-Date", "20150830T123600Z"}};
/// assert(sourcemeta::core::http_aws_sigv4_signed_headers(headers) ==
///        "host;x-amz-date");
/// ```
auto SOURCEMETA_CORE_HTTP_EXPORT http_aws_sigv4_signed_headers(
    const std::span<const std::pair<std::string_view, std::string_view>>
        headers) -> std::string;

/// @ingroup http
/// Compute the AWS Signature Version 4 credential scope from a date, region,
/// and service. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::http_aws_sigv4_credential_scope(
///            "20150830", "us-east-1", "service") ==
///        "20150830/us-east-1/service/aws4_request");
/// ```
auto SOURCEMETA_CORE_HTTP_EXPORT http_aws_sigv4_credential_scope(
    const std::string_view date, const std::string_view region,
    const std::string_view service) -> std::string;

/// @ingroup http
/// Compute the AWS Signature Version 4 string to sign from a timestamp, a
/// credential scope, and a canonical request. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// const auto result{sourcemeta::core::http_aws_sigv4_string_to_sign(
///     "20150830T123600Z", "20150830/us-east-1/service/aws4_request",
///     "canonical request")};
/// assert(result.starts_with("AWS4-HMAC-SHA256\n20150830T123600Z\n"));
/// ```
auto SOURCEMETA_CORE_HTTP_EXPORT http_aws_sigv4_string_to_sign(
    const std::string_view amz_date, const std::string_view scope,
    const std::string_view canonical_request) -> std::string;

/// @ingroup http
/// Derive the AWS Signature Version 4 signing key from a secret access key,
/// date, region, and service. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// const auto key{sourcemeta::core::http_aws_sigv4_signing_key(
///     "wJalrXUtnFEMI/K7MDENG+bPxRfiCYEXAMPLEKEY", "20150830", "us-east-1",
///     "service")};
/// assert(key.size() == 32);
/// ```
auto SOURCEMETA_CORE_HTTP_EXPORT http_aws_sigv4_signing_key(
    const std::string_view secret, const std::string_view date,
    const std::string_view region, const std::string_view service)
    -> std::array<std::uint8_t, 32>;

/// @ingroup http
/// Compute the AWS Signature Version 4 hex signature from a signing key and a
/// string to sign. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// const auto key{sourcemeta::core::http_aws_sigv4_signing_key(
///     "wJalrXUtnFEMI/K7MDENG+bPxRfiCYEXAMPLEKEY", "20150830", "us-east-1",
///     "service")};
/// assert(sourcemeta::core::http_aws_sigv4_signature(key, "string to sign")
///            .size() == 64);
/// ```
auto SOURCEMETA_CORE_HTTP_EXPORT
http_aws_sigv4_signature(const std::array<std::uint8_t, 32> &signing_key,
                         const std::string_view string_to_sign) -> std::string;

/// @ingroup http
/// Assemble the AWS Signature Version 4 `Authorization` header value from an
/// access key, credential scope, signed headers, and signature. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// const auto header{sourcemeta::core::http_aws_sigv4_authorization(
///     "AKIDEXAMPLE", "20150830/us-east-1/service/aws4_request",
///     "host;x-amz-date", "signature")};
/// assert(header.starts_with("AWS4-HMAC-SHA256 Credential=AKIDEXAMPLE/"));
/// ```
auto SOURCEMETA_CORE_HTTP_EXPORT http_aws_sigv4_authorization(
    const std::string_view access_key_id, const std::string_view scope,
    const std::string_view signed_headers, const std::string_view signature)
    -> std::string;

} // namespace sourcemeta::core

#endif
