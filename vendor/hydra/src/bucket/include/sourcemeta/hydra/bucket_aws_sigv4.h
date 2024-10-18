#ifndef SOURCEMETA_HYDRA_BUCKET_AWS_SIGV4_H
#define SOURCEMETA_HYDRA_BUCKET_AWS_SIGV4_H

#ifndef SOURCEMETA_HYDRA_BUCKET_EXPORT
#include <sourcemeta/hydra/bucket_export.h>
#endif

#include <sourcemeta/hydra/http.h>
#include <sourcemeta/jsontoolkit/uri.h>

#include <chrono>      // std::chrono::system_clock::time_point
#include <map>         // std::map
#include <ostream>     // std::ostream
#include <string>      // std::string
#include <string_view> // std::string_view

// This entire module is considered to be private and should not be directly
// used by consumers of this project
#if !defined(DOXYGEN)

namespace sourcemeta::hydra {

auto SOURCEMETA_HYDRA_BUCKET_EXPORT
aws_sigv4(const http::Method method, const sourcemeta::jsontoolkit::URI &url,
          std::string_view access_key, std::string_view secret_key,
          std::string_view region, std::string &&content_checksum,
          const std::chrono::system_clock::time_point now)
    -> std::map<std::string, std::string>;

auto SOURCEMETA_HYDRA_BUCKET_EXPORT aws_sigv4_scope(std::string_view datastamp,
                                                    std::string_view region,
                                                    std::ostream &output)
    -> void;

auto SOURCEMETA_HYDRA_BUCKET_EXPORT aws_sigv4_key(std::string_view secret_key,
                                                  std::string_view region,
                                                  std::string_view datastamp)
    -> std::string;

auto SOURCEMETA_HYDRA_BUCKET_EXPORT
aws_sigv4_canonical(const http::Method method, std::string_view host,
                    std::string_view path, std::string_view content_checksum,
                    std::string_view timestamp) -> std::string;

// A datestamp format is as follows:
// YYYY: Four-digit year
// MM: Two-digit month (01-12)
// DD: Two-digit day (01-31)
// Example: 20230913 (September 13, 2023)
auto SOURCEMETA_HYDRA_BUCKET_EXPORT aws_sigv4_datastamp(
    const std::chrono::system_clock::time_point time, std::ostream &output)
    -> void;

auto SOURCEMETA_HYDRA_BUCKET_EXPORT aws_sigv4_iso8601(
    const std::chrono::system_clock::time_point time, std::ostream &output)
    -> void;

} // namespace sourcemeta::hydra

#endif

#endif
