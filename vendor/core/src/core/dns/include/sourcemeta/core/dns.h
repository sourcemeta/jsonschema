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
/// RFC 952 to allow either a letter or a digit. This matches the
/// definition used by the JSON Schema `hostname` format. For example:
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
/// perform A-label or Punycode decoding. Those belong to the separate
/// `idn-hostname` format.
SOURCEMETA_CORE_DNS_EXPORT
auto is_hostname(const std::string_view value) -> bool;

} // namespace sourcemeta::core

#endif
