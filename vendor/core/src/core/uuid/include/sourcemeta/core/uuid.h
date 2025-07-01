#ifndef SOURCEMETA_CORE_UUID_H_
#define SOURCEMETA_CORE_UUID_H_

#ifndef SOURCEMETA_CORE_UUID_EXPORT
#include <sourcemeta/core/uuid_export.h>
#endif

#include <string> // std::string

/// @defgroup uuid UUID
/// @brief A growing implementation of RFC 9562 UUID.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/uuid.h>
/// ```

namespace sourcemeta::core {

/// @ingroup uuid
/// Generate a random UUID v4 string. For example:
///
/// ```cpp
/// #include <sourcemeta/core/uuid.h>
/// #include <iostream>
///
/// std::cout << sourcemeta::core::uuidv4() << "\n";
/// ```
///
/// See https://www.rfc-editor.org/rfc/rfc9562#name-uuid-version-4
SOURCEMETA_CORE_UUID_EXPORT auto uuidv4() -> std::string;

} // namespace sourcemeta::core

#endif
