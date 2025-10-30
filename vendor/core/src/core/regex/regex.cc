#include <sourcemeta/core/regex.h>

#include <pcre2.h>

#include "preprocess.h"

#include <cassert> // assert
#include <cstdint> // std::uint64_t
#include <regex>   // std::regex, std::smatch, std::regex_match
#include <string>  // std::string, std::stoull

namespace sourcemeta::core {

auto to_regex(const std::string &pattern) -> std::optional<Regex> {
  if (pattern == ".*" || pattern == "^.*$" || pattern == "^(.*)$" ||
      pattern == "(.*)" || pattern == "[\\s\\S]*" || pattern == "^[\\s\\S]*$") {
    return RegexTypeNoop{};

    // Note that the JSON Schema specification does not impose the use of any
    // regular expression flag. Given popular adoption, we assume `.` matches
    // new line characters (as in the `DOTALL`) option
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

  const std::string pcre2_pattern{preprocess_regex(pattern)};
  int pcre2_error_code{0};
  PCRE2_SIZE pcre2_error_offset{0};
  pcre2_code *pcre2_regex_raw{pcre2_compile(
      reinterpret_cast<PCRE2_SPTR>(pcre2_pattern.c_str()), pcre2_pattern.size(),
      PCRE2_UTF | PCRE2_UCP | PCRE2_NO_AUTO_CAPTURE | PCRE2_DOTALL |
          PCRE2_DOLLAR_ENDONLY | PCRE2_NEVER_BACKSLASH_C | PCRE2_NO_UTF_CHECK,
      &pcre2_error_code, &pcre2_error_offset, nullptr)};

  if (pcre2_regex_raw != nullptr) {
    std::shared_ptr<pcre2_code> pcre2_regex{pcre2_regex_raw, pcre2_code_free};
    pcre2_jit_compile(pcre2_regex.get(), PCRE2_JIT_COMPLETE);
    return RegexTypePCRE2{std::shared_ptr<void>(pcre2_regex)};
  }

  return std::nullopt;
}

auto matches(const Regex &regex, const std::string &value) -> bool {
  switch (static_cast<RegexIndex>(regex.index())) {
    case RegexIndex::Prefix:
      return value.starts_with(*std::get_if<RegexTypePrefix>(&regex));
    case RegexIndex::NonEmpty:
      return !value.empty();
    case RegexIndex::Range:
      return value.size() >= std::get_if<RegexTypeRange>(&regex)->first &&
             value.size() <= std::get_if<RegexTypeRange>(&regex)->second;
    case RegexIndex::PCRE2: {
      const RegexTypePCRE2 *pcre2_regex{std::get_if<RegexTypePCRE2>(&regex)};
      auto *pcre2_code_ptr{static_cast<pcre2_code *>(pcre2_regex->code.get())};
      // Re-use this to avoid creating and destroying the `struct`on every call
      thread_local pcre2_match_data *match_data{
          pcre2_match_data_create(1, nullptr)};
      const int match_result{pcre2_match(
          pcre2_code_ptr, reinterpret_cast<PCRE2_SPTR>(value.c_str()),
          value.size(), 0, PCRE2_NO_UTF_CHECK, match_data, nullptr)};
      return match_result >= 0;
    }
    case RegexIndex::Noop:
      return true;
  }

#if defined(_MSC_VER) && !defined(__clang__)
  __assume(false);
#else
  __builtin_unreachable();
#endif
}

auto matches_if_valid(const std::string &pattern, const std::string &value)
    -> bool {
  const auto regex{to_regex(pattern)};
  return regex.has_value() && matches(regex.value(), value);
}

} // namespace sourcemeta::core
