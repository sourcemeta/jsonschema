#ifndef SOURCEMETA_CORE_EMAIL_H_
#define SOURCEMETA_CORE_EMAIL_H_

#ifndef SOURCEMETA_CORE_EMAIL_EXPORT
#include <sourcemeta/core/email_export.h>
#endif

#include <string_view> // std::string_view

/// @defgroup email Email
/// @brief E-mail address validation per RFC 5321.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/email.h>
/// ```

namespace sourcemeta::core {

/// @ingroup email
/// Check whether the given string is a valid `Mailbox` per RFC 5321
/// Section 4.1.2, under the length constraints from Section 4.5.3.1.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/email.h>
///
/// #include <cassert>
///
/// assert(sourcemeta::core::is_email("user@example.com"));
/// assert(sourcemeta::core::is_email("\"a b\"@example.com"));
/// assert(sourcemeta::core::is_email("user@[192.168.1.1]"));
/// assert(!sourcemeta::core::is_email("plain"));
/// ```
SOURCEMETA_CORE_EMAIL_EXPORT
auto is_email(const std::string_view value) -> bool;

} // namespace sourcemeta::core

#endif
