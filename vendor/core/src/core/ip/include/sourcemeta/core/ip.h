#ifndef SOURCEMETA_CORE_IP_H_
#define SOURCEMETA_CORE_IP_H_

#ifndef SOURCEMETA_CORE_IP_EXPORT
#include <sourcemeta/core/ip_export.h>
#endif

#include <string_view> // std::string_view

/// @defgroup ip IP
/// @brief IPv4 (RFC 3986) and IPv6 (RFC 3986, RFC 4291) address validation.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/ip.h>
/// ```

namespace sourcemeta::core {

/// @ingroup ip
/// Check whether the given string is a valid IPv4 address per RFC 3986
/// Section 3.2.2. For example:
///
/// ```cpp
/// #include <sourcemeta/core/ip.h>
///
/// #include <cassert>
///
/// assert(sourcemeta::core::is_ipv4("192.168.1.1"));
/// assert(!sourcemeta::core::is_ipv4("999.0.0.1"));
/// ```
SOURCEMETA_CORE_IP_EXPORT
auto is_ipv4(std::string_view address) -> bool;

/// @ingroup ip
/// Check whether the given string is a valid IPv6 address per RFC 3986
/// Section 3.2.2 and RFC 4291 Section 2.2. The input must not include
/// surrounding brackets. For example:
///
/// ```cpp
/// #include <sourcemeta/core/ip.h>
///
/// #include <cassert>
///
/// assert(sourcemeta::core::is_ipv6("2001:db8::1"));
/// assert(!sourcemeta::core::is_ipv6("not an address"));
/// ```
SOURCEMETA_CORE_IP_EXPORT
auto is_ipv6(std::string_view address) -> bool;

} // namespace sourcemeta::core

#endif
