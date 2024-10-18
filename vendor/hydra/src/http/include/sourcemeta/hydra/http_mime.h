#ifndef SOURCEMETA_HYDRA_HTTP_MIME_H
#define SOURCEMETA_HYDRA_HTTP_MIME_H

#ifndef SOURCEMETA_HYDRA_HTTP_EXPORT
#include <sourcemeta/hydra/http_export.h>
#endif

#include <filesystem> // std::filesystem
#include <string>     // std::string

namespace sourcemeta::hydra {

/// @ingroup http
/// Find a IANA MIME type for the given file. For example:
///
/// ```cpp
/// #include <sourcemeta/hydra/http.h>
/// #include <cassert>
///
/// const auto result{sourcemeta::hydra::mime_type("image.png")};
/// assert(result == "image/png");
/// ```
auto SOURCEMETA_HYDRA_HTTP_EXPORT
mime_type(const std::filesystem::path &file_path) -> std::string;

} // namespace sourcemeta::hydra

#endif
