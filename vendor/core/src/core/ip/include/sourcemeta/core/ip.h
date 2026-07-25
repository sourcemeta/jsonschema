#ifndef SOURCEMETA_CORE_IP_H_
#define SOURCEMETA_CORE_IP_H_

#ifndef SOURCEMETA_CORE_IP_EXPORT
#include <sourcemeta/core/ip_export.h>
#endif

#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional
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

/// @ingroup ip
/// The reachability class of an IP address (RFC 6890).
enum class IPAddressClass : std::uint8_t {
  /// A globally reachable address.
  Public,
  /// A loopback address (RFC 1122, RFC 4291 Section 2.5.3).
  Loopback,
  /// A private-use or shared address (RFC 1918, RFC 6598).
  Private,
  /// A link-local address (RFC 3927, RFC 4291 Section 2.5.6).
  LinkLocal,
  /// A unique local address (RFC 4193).
  UniqueLocal,
  /// A multicast address (RFC 5771, RFC 4291 Section 2.7).
  Multicast,
  /// An address reserved for future or special use (RFC 1112, RFC 5737).
  Reserved,
  /// The unspecified address (RFC 4291 Section 2.5.2) or the "this host"
  /// network (RFC 1122).
  Unspecified
};

/// @ingroup ip
/// Classify an IPv4 address (RFC 6890), returning no value when the input is
/// not a valid IPv4 address. For example:
///
/// ```cpp
/// #include <sourcemeta/core/ip.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::ipv4_classify("127.0.0.1").value() ==
///        sourcemeta::core::IPAddressClass::Loopback);
/// ```
SOURCEMETA_CORE_IP_EXPORT
auto ipv4_classify(const std::string_view address)
    -> std::optional<IPAddressClass>;

/// @ingroup ip
/// Classify an IPv6 address (RFC 6890), returning no value when the input is
/// not a valid IPv6 address. An IPv4-mapped or IPv4-compatible address is
/// classified as its embedded IPv4 address. For example:
///
/// ```cpp
/// #include <sourcemeta/core/ip.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::ipv6_classify("::1").value() ==
///        sourcemeta::core::IPAddressClass::Loopback);
/// ```
SOURCEMETA_CORE_IP_EXPORT
auto ipv6_classify(const std::string_view address)
    -> std::optional<IPAddressClass>;

/// @ingroup ip
/// Whether an IPv4 or IPv6 address is globally reachable (RFC 6890), returning
/// false both for an address that is not and for input that is not a valid
/// address. For example:
///
/// ```cpp
/// #include <sourcemeta/core/ip.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::ip_is_globally_routable("8.8.8.8"));
/// assert(!sourcemeta::core::ip_is_globally_routable("10.0.0.1"));
/// ```
SOURCEMETA_CORE_IP_EXPORT
auto ip_is_globally_routable(const std::string_view address) -> bool;

} // namespace sourcemeta::core

#endif
