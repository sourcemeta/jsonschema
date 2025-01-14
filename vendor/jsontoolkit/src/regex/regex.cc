#include <sourcemeta/jsontoolkit/regex.h>

#include <cassert> // assert
#include <string>  // std::stoull

namespace sourcemeta::jsontoolkit {

auto to_regex(const JSON::String &pattern) -> std::optional<Regex> {
  if (pattern == ".*" || pattern == "^.*$" || pattern == "^(.*)$" ||
      pattern == "(.*)" || pattern == "[\\s\\S]*" || pattern == "^[\\s\\S]*$") {
    return RegexTypeNoop{};
  } else if (pattern == ".+" || pattern == "^.+$" || pattern == "^(.+)$" ||
             pattern == ".") {
    return RegexTypeNonEmpty{};
  }

  const std::regex PREFIX_REGEX{R"(^\^([a-zA-Z0-9-_/@]+)(\.\*)?)"};
  std::smatch matches_prefix;
  if (std::regex_match(pattern, matches_prefix, PREFIX_REGEX)) {
    return RegexTypePrefix{matches_prefix[1].str()};
  }

  const std::regex RANGE_REGEX{R"(^\^\.\{(\d+),(\d+)\}\$$)"};
  std::smatch matches_range;
  if (std::regex_match(pattern, matches_range, RANGE_REGEX)) {
    const std::uint64_t minimum{std::stoull(matches_range[1].str())};
    const std::uint64_t maximum{std::stoull(matches_range[2].str())};
    assert(minimum <= maximum);
    return RegexTypeRange{minimum, maximum};
  }

  boost::regex result{
      pattern,
      boost::regex::no_except |
          // See https://en.cppreference.com/w/cpp/regex/basic_regex/constants
          boost::regex::ECMAScript |

          // When performing matches, all marked sub-expressions (expr) are
          // treated as non-marking sub-expressions (?:expr)
          boost::regex::nosubs |

          // Instructs the regular expression engine to make matching faster,
          // with the potential cost of making construction slower
          boost::regex::optimize};

  // Returns zero if the expression contains a valid regular expression
  // See
  // https://www.boost.org/doc/libs/1_82_0/libs/regex/doc/html/boost_regex/ref/basic_regex.html
  if (result.status() == 0) {
    return result;
  }

  try {
    // Boost seems to sometimes be overly strict, so we still default to
    // the standard implementation
    return std::regex{
        pattern,
        // See https://en.cppreference.com/w/cpp/regex/basic_regex/constants
        std::regex::ECMAScript |

            // When performing matches, all marked sub-expressions (expr) are
            // treated as non-marking sub-expressions (?:expr)
            std::regex::nosubs |

            // Instructs the regular expression engine to make matching
            // faster, with the potential cost of making construction slower
            std::regex::optimize};
  } catch (const std::regex_error &) {
    return std::nullopt;
  }
}

auto matches(const Regex &regex, const JSON::String &value) -> bool {
  switch (static_cast<RegexIndex>(regex.index())) {
    case RegexIndex::Boost:
      return boost::regex_search(value, *std::get_if<RegexTypeBoost>(&regex));
    case RegexIndex::Prefix:
      return value.starts_with(*std::get_if<RegexTypePrefix>(&regex));
    case RegexIndex::NonEmpty:
      return !value.empty();
    case RegexIndex::Range:
      return value.size() >= std::get_if<RegexTypeRange>(&regex)->first &&
             value.size() <= std::get_if<RegexTypeRange>(&regex)->second;
    case RegexIndex::Std:
      return std::regex_search(value, *std::get_if<RegexTypeStd>(&regex));
    case RegexIndex::Noop:
      return true;
  }

    // See https://en.cppreference.com/w/cpp/utility/unreachable
#if defined(_MSC_VER) && !defined(__clang__)
  __assume(false);
#else
  __builtin_unreachable();
#endif
}

} // namespace sourcemeta::jsontoolkit
