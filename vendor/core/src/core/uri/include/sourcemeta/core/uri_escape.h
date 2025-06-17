#ifndef SOURCEMETA_CORE_URI_ESCAPE_H_
#define SOURCEMETA_CORE_URI_ESCAPE_H_

#ifndef SOURCEMETA_CORE_URI_EXPORT
#include <sourcemeta/core/uri_export.h>
#endif

#include <istream> // std::istream
#include <ostream> // std::ostream

namespace sourcemeta::core {

/// @ingroup uri
enum class URIEscapeMode {
  // Escape every characted that is not in the URI "unreserved" ABNF category
  // See https://www.rfc-editor.org/rfc/rfc3986#appendix-A
  SkipUnreserved,
  // Escape every characted that is not in either the URI "unreserved" nor
  // "sub-delims" ABNF categories
  // See https://www.rfc-editor.org/rfc/rfc3986#appendix-A
  SkipSubDelims
};

/// @ingroup uri
///
/// Escape a character as established by RFC 3986 using C++ standard streams.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/uri.h>
/// #include <sstream>
/// #include <cassert>
///
/// std::ostringstream output;
/// sourcemeta::core::uri_escape(' ', output,
///   sourcemeta::core::URIEscapeMode::SkipUnreserved);
/// assert(output.str() == "%20");
/// ```
SOURCEMETA_CORE_URI_EXPORT
auto uri_escape(const char character, std::ostream &output,
                const URIEscapeMode mode) -> void;

/// @ingroup uri
///
/// Escape a string as established by RFC 3986 using C++ standard streams. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/uri.h>
/// #include <sstream>
/// #include <cassert>
///
/// std::istringstream input{"foo bar"};
/// std::ostringstream output;
/// sourcemeta::core::uri_escape(input, output,
///   sourcemeta::core::URIEscapeMode::SkipUnreserved);
/// assert(output.str() == "foo%20bar");
/// ```
SOURCEMETA_CORE_URI_EXPORT
auto uri_escape(std::istream &input, std::ostream &output,
                const URIEscapeMode mode) -> void;

/// @ingroup uri
///
/// Unescape a string that has been percentage-escaped as established by
/// RFC 3986 using C++ standard streams. For example:
///
/// ```cpp
/// #include <sourcemeta/core/uri.h>
/// #include <sstream>
/// #include <cassert>
///
/// std::istringstream input{"foo%20bar"};
/// std::ostringstream output;
/// sourcemeta::core::uri_unescape(input, output);
/// assert(output.str() == "foo bar");
/// ```
SOURCEMETA_CORE_URI_EXPORT
auto uri_unescape(std::istream &input, std::ostream &output) -> void;

} // namespace sourcemeta::core

#endif
