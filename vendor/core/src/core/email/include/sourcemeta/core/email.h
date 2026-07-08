#ifndef SOURCEMETA_CORE_EMAIL_H_
#define SOURCEMETA_CORE_EMAIL_H_

#ifndef SOURCEMETA_CORE_EMAIL_EXPORT
#include <sourcemeta/core/email_export.h>
#endif

#include <string_view> // std::string_view

/// @defgroup email Email
/// @brief E-mail address validation per RFC 5321 and RFC 6531.
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

/// @ingroup email
/// Check whether the given string is a valid internationalized `Mailbox`
/// per RFC 6531 Section 3.3 (extended Mailbox address syntax). Beyond the
/// ASCII grammar accepted by `is_email`, the local-part atoms, quoted
/// content, and domain labels may also contain valid UTF-8 non-ASCII byte
/// sequences (RFC 6532 Section 3.1). For example:
///
/// ```cpp
/// #include <sourcemeta/core/email.h>
///
/// #include <cassert>
///
/// assert(sourcemeta::core::is_idn_email(
///     "\xec\x8b\xa4\xeb\xa1\x80@\xec\x8b\xa4\xeb\xa1\x80.\xed\x85\x8c\xec\x8a\xa4\xed\x8a\xb8"));
/// assert(sourcemeta::core::is_idn_email("joe.bloggs@example.com"));
/// assert(!sourcemeta::core::is_idn_email("2962"));
/// ```
SOURCEMETA_CORE_EMAIL_EXPORT
auto is_idn_email(const std::string_view value) -> bool;

/// @ingroup email
/// Check whether the given string is a valid internationalized `Mailbox` per
/// RFC 6531 Section 3.3, validating the domain under UTS #46 processing rather
/// than strict IDNA 2008. The domain is mapped (case folding, compatibility
/// mappings such as fullwidth to ASCII, and removal of ignorable characters)
/// and NFC-normalised before validation, so forms that strict validation
/// rejects, such as fullwidth characters and non-normalised (non-NFC) labels,
/// are accepted. The local part carries no normalisation requirement
/// (RFC 6531) and is validated as-is. See https://www.unicode.org/reports/tr46/
/// for the algorithm. For example:
///
/// ```cpp
/// #include <sourcemeta/core/email.h>
///
/// #include <cassert>
///
/// assert(sourcemeta::core::is_idn_email_uts46("joe.bloggs@example.com"));
/// // The fullwidth domain U+FF41 U+FF42 U+FF43 maps to "abc" and is accepted,
/// // whereas strict IDNA 2008 validation rejects it
/// assert(sourcemeta::core::is_idn_email_uts46(
///     "user@\xef\xbd\x81\xef\xbd\x82\xef\xbd\x83"));
/// assert(!sourcemeta::core::is_idn_email_uts46("2962"));
/// ```
SOURCEMETA_CORE_EMAIL_EXPORT
auto is_idn_email_uts46(const std::string_view value) -> bool;

} // namespace sourcemeta::core

#endif
