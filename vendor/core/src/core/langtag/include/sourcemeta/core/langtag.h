#ifndef SOURCEMETA_CORE_LANGTAG_H_
#define SOURCEMETA_CORE_LANGTAG_H_

#ifndef SOURCEMETA_CORE_LANGTAG_EXPORT
#include <sourcemeta/core/langtag_export.h>
#endif

#include <string_view> // std::string_view

/// @defgroup langtag LangTag
/// @brief BCP 47 language tag validation utilities.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/langtag.h>
/// ```

namespace sourcemeta::core {

/// @ingroup langtag
/// Check whether the given string is a well-formed language tag per RFC 5646
/// (BCP 47). In addition to the grammar, the two duplication errors that the
/// specification forbids without consulting any registry are rejected: a
/// repeated variant subtag (RFC 5646 Section 2.2.5) and more than one extension
/// for the same singleton (RFC 5646 Section 2.2.6). Validity against the IANA
/// Language Subtag Registry is not checked, so a structurally well-formed tag
/// whose subtags are not registered is still accepted. Comparison is
/// case-insensitive, as language tags are. For example:
///
/// ```cpp
/// #include <sourcemeta/core/langtag.h>
///
/// #include <cassert>
///
/// assert(sourcemeta::core::is_langtag("en"));
/// assert(sourcemeta::core::is_langtag("zh-Hant-HK"));
/// assert(sourcemeta::core::is_langtag("x-private"));
/// assert(!sourcemeta::core::is_langtag("en-"));
/// assert(!sourcemeta::core::is_langtag("de-1996-1996"));
/// ```
SOURCEMETA_CORE_LANGTAG_EXPORT
auto is_langtag(const std::string_view value) -> bool;

} // namespace sourcemeta::core

#endif
