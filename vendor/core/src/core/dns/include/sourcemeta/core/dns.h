#ifndef SOURCEMETA_CORE_DNS_H_
#define SOURCEMETA_CORE_DNS_H_

#ifndef SOURCEMETA_CORE_DNS_EXPORT
#include <sourcemeta/core/dns_export.h>
#endif

#include <string_view> // std::string_view

/// @defgroup dns DNS
/// @brief DNS and hostname validation utilities.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/dns.h>
/// ```

namespace sourcemeta::core {

/// @ingroup dns
/// Check whether the given string is a valid Internet host name per
/// RFC 1123 Section 2.1, which relaxes the first-character rule of
/// RFC 952 to allow either a letter or a digit. For example:
///
/// ```cpp
/// #include <sourcemeta/core/dns.h>
///
/// #include <cassert>
///
/// assert(sourcemeta::core::is_hostname("www.example.com"));
/// assert(sourcemeta::core::is_hostname("1host"));
/// assert(!sourcemeta::core::is_hostname("-bad"));
/// assert(!sourcemeta::core::is_hostname("example."));
/// ```
///
/// This function implements RFC 1123 §2.1 (ASCII only). It does not
/// perform A-label or Punycode decoding. For internationalized host
/// names see `is_idn_hostname`.
SOURCEMETA_CORE_DNS_EXPORT
auto is_hostname(const std::string_view value) -> bool;

/// @ingroup dns
/// Check whether the given string is a valid internationalized host name per
/// RFC 5891 Section 4. Each label is validated as an RFC 5890 A-label or
/// U-label (with RFC 5892 ContextJ and ContextO contextual rules and the
/// RFC 5891 §4.1.2.A NFC requirement), and the RFC 5893 Bidi rule is
/// enforced on every label of a Bidi domain name. For example:
///
/// ```cpp
/// #include <sourcemeta/core/dns.h>
///
/// #include <cassert>
///
/// assert(sourcemeta::core::is_idn_hostname("www.example.com"));
/// assert(sourcemeta::core::is_idn_hostname(
///     "\xec\x8b\xa4\xeb\xa1\x80.\xed\x85\x8c\xec\x8a\xa4\xed\x8a\xb8"));
/// assert(!sourcemeta::core::is_idn_hostname("-bad"));
/// ```
SOURCEMETA_CORE_DNS_EXPORT
auto is_idn_hostname(const std::string_view value) -> bool;

} // namespace sourcemeta::core

#endif
