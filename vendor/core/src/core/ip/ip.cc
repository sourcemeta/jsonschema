#include <sourcemeta/core/ip.h>

#include <string_view> // std::string_view

namespace sourcemeta::core {

auto ip_is_globally_routable(const std::string_view address) -> bool {
  return ipv4_classify(address) == IPAddressClass::Public ||
         ipv6_classify(address) == IPAddressClass::Public;
}

} // namespace sourcemeta::core
