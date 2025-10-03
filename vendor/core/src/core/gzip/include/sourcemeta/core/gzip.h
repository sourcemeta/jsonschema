#ifndef SOURCEMETA_CORE_GZIP_H_
#define SOURCEMETA_CORE_GZIP_H_

#ifndef SOURCEMETA_CORE_GZIP_EXPORT
#include <sourcemeta/core/gzip_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/gzip_error.h>
#include <sourcemeta/core/gzip_tar.h>
// NOLINTEND(misc-include-cleaner)

#include <istream>     // std::istream
#include <ostream>     // std::ostream
#include <string>      // std::string
#include <string_view> // std::string_view

/// @defgroup gzip GZIP
/// @brief An implementation of RFC 1952 GZIP.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/gzip.h>
/// ```

namespace sourcemeta::core {

/// @ingroup gzip
///
/// Compress an input stream into an output stream. For example:
///
/// ```cpp
/// #include <sourcemeta/core/gzip.h>
/// #include <cassert>
/// #include <sstream>
///
/// std::istringstream input{"Hello World"};
/// std::ostringstream output;
/// sourcemeta::core::gzip(input, output);
/// assert(!output.str().empty());
/// ```
SOURCEMETA_CORE_GZIP_EXPORT auto gzip(std::istream &input, std::ostream &output)
    -> void;

/// @ingroup gzip
///
/// A convenience function to compress an input stream into a sequence of
/// bytes represented using a string. For example:
///
/// ```cpp
/// #include <sourcemeta/core/gzip.h>
/// #include <cassert>
/// #include <sstream>
///
/// std::istringstream stream{"Hello World"};
/// const auto result{sourcemeta::core::gzip(stream)};
/// assert(result == "Hello World");
/// ```
SOURCEMETA_CORE_GZIP_EXPORT auto gzip(std::istream &stream) -> std::string;

/// @ingroup gzip
///
/// A convenience function to compress an input string into a sequence of bytes
/// represented using a string. For example:
///
/// ```cpp
/// #include <sourcemeta/core/gzip.h>
/// #include <cassert>
///
/// const auto result{sourcemeta::core::gzip("Hello World")};
/// assert(!result.empty());
/// ```
SOURCEMETA_CORE_GZIP_EXPORT auto gzip(const std::string &input) -> std::string;

/// @ingroup gzip
///
/// Decompress an input stream into an output stream. For example:
///
/// ```cpp
/// #include <sourcemeta/core/gzip.h>
/// #include <cassert>
/// #include <sstream>
///
/// std::istringstream input{sourcemeta::core::gzip("Hello World")};
/// std::ostringstream output;
/// sourcemeta::core::gunzip(input, output);
/// assert(output.str() == "Hello World");
/// ```
SOURCEMETA_CORE_GZIP_EXPORT auto gunzip(std::istream &input,
                                        std::ostream &output) -> void;

/// @ingroup gzip
///
/// A convenience function to decompress an input stream into a sequence of
/// bytes represented using a string. For example:
///
/// ```cpp
/// #include <sourcemeta/core/gzip.h>
/// #include <cassert>
/// #include <sstream>
///
/// const auto input{sourcemeta::core::gzip("Hello World")};
/// std::istringstream stream{input};
/// const auto result{sourcemeta::core::gunzip(stream)};
/// assert(result == "Hello World");
/// ```
SOURCEMETA_CORE_GZIP_EXPORT auto gunzip(std::istream &stream) -> std::string;

/// @ingroup gzip
///
/// A convenience function to decompress an input string into a sequence of
/// bytes represented using a string. For example:
///
/// ```cpp
/// #include <sourcemeta/core/gzip.h>
/// #include <cassert>
///
/// const auto result{sourcemeta::core::gunzip("Hello World")};
/// assert(result == "Hello World");
/// ```
SOURCEMETA_CORE_GZIP_EXPORT auto gunzip(const std::string &input)
    -> std::string;

} // namespace sourcemeta::core

#endif
