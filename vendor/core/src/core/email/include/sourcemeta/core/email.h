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
///
/// The domain is validated slightly more strictly than the bare RFC 5321
/// Section 4.1.2 grammar, and one address-literal case follows the section
/// prose over its ABNF. Both are deliberate stances rather than accidents.
///
/// A domain label beginning with the `xn--` ACE prefix must be a valid IDNA
/// A-label, not merely a letter-digit-hyphen string. The Section 4.1.2 grammar
/// would accept any such label, but one that does not decode to a real
/// internationalized label can never name a deliverable domain, so it is
/// rejected. This is intentionally stricter than the grammar.
///
/// An `[IPv6:...]` address literal is validated per RFC 4291. The Section 4.1.3
/// prose specifies the IPv6 syntax as that of RFC 4291, while the `IPv6-addr`
/// ABNF in the same section is stricter and conflicts with it, so the prose is
/// followed. A bracketed `[IPv6:...]` whose body is not a valid address is
/// still accepted, because Section 4.1.3 also permits any
/// General-address-literal (a registered tag, a colon, and content) and ABNF
/// alternatives are unordered.
///
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
