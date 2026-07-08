#include <sourcemeta/core/http.h>

#include "helpers.h"

#include <chrono>      // std::chrono::seconds
#include <cstddef>     // std::size_t
#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto http_cache_control_max_age(const std::string_view cache_control) noexcept
    -> std::optional<std::chrono::seconds> {
  // RFC 9111 §1.2.2: a delta-seconds value larger than the cache can represent
  // is treated as 2^31
  constexpr std::chrono::seconds::rep overflow_seconds{2147483648};
  std::optional<std::chrono::seconds> result;
  bool found{false};
  http_for_each_list_entry(
      cache_control, [&](const std::string_view directive) -> void {
        if (found) {
          return;
        }

        const auto separator{directive.find('=')};
        if (separator == std::string_view::npos ||
            !equals_ignore_case(http_subview(directive, 0, separator),
                                "max-age")) {
          return;
        }

        found = true;
        auto value{http_subview(directive, separator + 1,
                                directive.size() - separator - 1)};
        // RFC 9111 §5.2.2.1 forbids a sender from generating the quoted-string
        // form for delta-seconds, but RFC 9111 §5.2.4 asks recipients to accept
        // it regardless
        const bool quoted{value.size() >= 2 && value.front() == '"' &&
                          value.back() == '"'};
        if (quoted) {
          value = http_subview(value, 1, value.size() - 2);
        }

        if (value.empty()) {
          return;
        }

        std::chrono::seconds::rep seconds{0};
        for (std::size_t index{0}; index < value.size(); ++index) {
          char character{value[index]};
          // RFC 9110 §5.6.4: within a quoted-string a backslash escapes the
          // octet that follows it
          if (quoted && character == '\\') {
            index += 1;
            if (index >= value.size()) {
              return;
            }

            character = value[index];
          }

          if (character < '0' || character > '9') {
            return;
          }

          // Clamp before multiplying so the running total cannot itself
          // overflow on a hostile number of digits
          if (seconds > overflow_seconds / 10) {
            seconds = overflow_seconds;
            continue;
          }

          seconds = (seconds * 10) + (character - '0');
        }

        if (seconds > overflow_seconds) {
          seconds = overflow_seconds;
        }

        result = std::chrono::seconds{seconds};
      });

  return result;
}

} // namespace sourcemeta::core
