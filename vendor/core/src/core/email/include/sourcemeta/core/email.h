#ifndef SOURCEMETA_CORE_EMAIL_H_
#define SOURCEMETA_CORE_EMAIL_H_

#ifndef SOURCEMETA_CORE_EMAIL_EXPORT
#include <sourcemeta/core/email_export.h>
#endif

#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

/// @defgroup email Email
/// @brief E-mail address validation per RFC 5321 and RFC 6531, plus
/// canonical account identity IRIs per RFC 6068 and RFC 7565.
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
/// turned down rather than read as a General-address-literal, since the tag
/// names the syntax that has to follow it. That general form is not accepted
/// under any other tag either, as Section 4.1.3 requires a Standardized-tag to
/// be registered with IANA before being used and that registry carries the
/// IPv6 tag alone.
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

/// @ingroup email
/// Produce the domain half of a valid RFC 5321 `Mailbox`, returning an empty
/// view when the input is not one, which no valid mailbox can otherwise yield
/// since a domain is never empty. The separator is located by parsing the
/// address, since the first at sign does not reliably mark it: RFC 5321
/// Section 4.1.2 admits one inside a quoted local part. The
/// result borrows from the input and keeps its spelling, so a caller comparing
/// domains applies the RFC 5321 Section 2.4 case insensitivity itself. Only
/// the ASCII grammar is accepted, so an internationalized address per RFC 6531
/// yields nothing. For example:
///
/// ```cpp
/// #include <sourcemeta/core/email.h>
///
/// #include <cassert>
///
/// assert(sourcemeta::core::email_domain("\"not@me\"@example.org") ==
///        "example.org");
/// assert(sourcemeta::core::email_domain("plain").empty());
/// ```
SOURCEMETA_CORE_EMAIL_EXPORT
auto email_domain(const std::string_view value) -> std::string_view;

/// @ingroup email
/// Produce the canonical RFC 6068 `mailto` IRI that identifies the given
/// RFC 5321 `Mailbox`, with no result when the input is not one. The domain
/// name is lowercased, the RFC 3986 Section 6.2.3 scheme-based case
/// normalization that names this very scheme in its example, while the local
/// part is case sensitive per RFC 5321 Section 2.4 and an address literal is
/// not a DNS name, so both keep their spelling. For example:
///
/// ```cpp
/// #include <sourcemeta/core/email.h>
///
/// #include <cassert>
///
/// const auto iri{sourcemeta::core::mailto_iri("gorby%kremvax@example.com")};
/// assert(iri.has_value());
/// assert(iri.value() == "mailto:gorby%25kremvax@example.com");
/// ```
SOURCEMETA_CORE_EMAIL_EXPORT
auto mailto_iri(const std::string_view value) -> std::optional<std::string>;

/// @ingroup email
/// Produce the canonical RFC 7565 `acct` IRI that identifies the given
/// `user@host` account, with no result when the input is not one. The host is
/// lowercased, as RFC 7565 Section 4 compares these IRIs under RFC 3986
/// Section 6.2.2.1 case normalization, while the account name keeps its case.
///
/// The account name is limited to the printable ASCII repertoire of the
/// PRECIS IdentifierClass (RFC 7564 Section 9.11) and the host to an ASCII
/// DNS name, so internationalized forms yield no result. This restriction is
/// deliberate and may be lifted later without re-minting any identity this
/// function already produces.
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/email.h>
///
/// #include <cassert>
///
/// const auto iri{sourcemeta::core::acct_iri(
///     "juliet@capulet.example@shoppingsite.example")};
/// assert(iri.has_value());
/// assert(iri.value() ==
///        "acct:juliet%40capulet.example@shoppingsite.example");
/// ```
SOURCEMETA_CORE_EMAIL_EXPORT
auto acct_iri(const std::string_view value) -> std::optional<std::string>;

} // namespace sourcemeta::core

#endif
