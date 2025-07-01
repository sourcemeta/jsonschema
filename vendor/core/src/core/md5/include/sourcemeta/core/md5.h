#ifndef SOURCEMETA_CORE_MD5_H_
#define SOURCEMETA_CORE_MD5_H_

#ifndef SOURCEMETA_CORE_MD5_EXPORT
#include <sourcemeta/core/md5_export.h>
#endif

#include <ostream>     // std::ostream
#include <string_view> // std::string_view

/// @defgroup md5 MD5
/// @brief An implementation of RFC 1321 MD5 Message-Digest.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/md5.h>
/// ```

namespace sourcemeta::core {

/// @ingroup md5
/// Hash a string using MD5. For example:
///
/// ```cpp
/// #include <sourcemeta/hydra/crypto.h>
/// #include <sstream>
/// #include <iostream>
///
/// std::ostringstream result;
/// sourcemeta::hydra::md5("foo bar", result);
/// std::cout << result.str() << "\n";
/// ```
auto SOURCEMETA_CORE_MD5_EXPORT md5(std::string_view input,
                                    std::ostream &output) -> void;

} // namespace sourcemeta::core

#endif
