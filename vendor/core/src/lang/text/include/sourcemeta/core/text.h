#ifndef SOURCEMETA_CORE_TEXT_H_
#define SOURCEMETA_CORE_TEXT_H_

#ifndef SOURCEMETA_CORE_TEXT_EXPORT
#include <sourcemeta/core/text_export.h>
#endif

#include <string> // std::string

/// @defgroup text Text
/// @brief A collection of general-purpose text manipulation utilities
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// ```

namespace sourcemeta::core {

/// @ingroup text
///
/// Convert a string to Title Case in place. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
/// #include <string>
///
/// std::string value{"hello_world"};
/// sourcemeta::core::to_title_case(value);
/// assert(value == "Hello World");
/// ```
SOURCEMETA_CORE_TEXT_EXPORT
auto to_title_case(std::string &value) -> void;

} // namespace sourcemeta::core

#endif
