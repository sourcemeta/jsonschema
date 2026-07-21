#include <sourcemeta/core/regex.h>
#include <sourcemeta/core/unicode.h>

#include <pcre2.h>

#include "iregexp.h"
#include "permissive.h"

#include <charconv>     // std::from_chars
#include <cstddef>      // std::size_t
#include <cstdint>      // std::uint32_t
#include <regex>        // std::regex, std::smatch, std::regex_match
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <system_error> // std::errc
#include <utility>      // std::unreachable

namespace sourcemeta::core {

namespace {

auto compile_pcre2(const std::string &pattern, const std::uint32_t options)
    -> std::optional<Regex> {
  int pcre2_error_code{0};
  PCRE2_SIZE pcre2_error_offset{0};
  pcre2_code *pcre2_regex_raw{pcre2_compile(
      reinterpret_cast<PCRE2_SPTR>(pattern.c_str()), pattern.size(), options,
      &pcre2_error_code, &pcre2_error_offset, nullptr)};
  if (pcre2_regex_raw == nullptr) {
    return std::nullopt;
  }

  std::shared_ptr<pcre2_code> pcre2_regex{pcre2_regex_raw, pcre2_code_free};
  pcre2_jit_compile(pcre2_regex.get(), PCRE2_JIT_COMPLETE);
  return RegexTypePCRE2{std::shared_ptr<void>(pcre2_regex)};
}

auto compiles_with_ecma_options(const std::string &pattern) -> bool {
  int pcre2_error_code{0};
  PCRE2_SIZE pcre2_error_offset{0};
  pcre2_code *pcre2_regex_raw{pcre2_compile(
      reinterpret_cast<PCRE2_SPTR>(pattern.c_str()), pattern.size(),
      // Capturing groups are kept enabled so that ECMA-262 numbered
      // backreferences like (a)\1 compile and match, at a small tracking cost
      PCRE2_UTF | PCRE2_UCP | PCRE2_DOTALL | PCRE2_DOLLAR_ENDONLY |
          PCRE2_NEVER_BACKSLASH_C | PCRE2_MATCH_INVALID_UTF |
          PCRE2_ALLOW_EMPTY_CLASS,
      &pcre2_error_code, &pcre2_error_offset, nullptr)};

  if (pcre2_regex_raw == nullptr) {
    return false;
  }

  pcre2_code_free(pcre2_regex_raw);
  return true;
}

auto lookbehinds_as_lookaheads(const std::string &pattern) -> std::string {
  std::string result;
  result.reserve(pattern.size());
  bool in_class{false};
  for (std::size_t position = 0; position < pattern.size(); ++position) {
    const char current{pattern[position]};
    if (current == '\\' && position + 1 < pattern.size()) {
      result += current;
      result += pattern[position + 1];
      position += 1;
      continue;
    }

    if (current == '[') {
      in_class = true;
    } else if (current == ']') {
      in_class = false;
    }

    if (!in_class && current == '(' && position + 3 < pattern.size() &&
        pattern[position + 1] == '?' && pattern[position + 2] == '<' &&
        (pattern[position + 3] == '=' || pattern[position + 3] == '!')) {
      result += "(?";
      result += pattern[position + 3];
      position += 3;
      continue;
    }

    result += current;
  }

  return result;
}

} // namespace

auto to_regex(const std::string_view pattern, const RegexDialect dialect)
    -> std::optional<Regex> {
  if (dialect != RegexDialect::Permissive) {
    const auto translated{
        translate_iregexp(pattern, dialect == RegexDialect::IRegexp)};
    if (!translated.has_value()) {
      return std::nullopt;
    }

    // Grouping in RFC 9485 carries no capturing semantics, as matching is
    // strictly Boolean, so capturing is disabled altogether
    return compile_pcre2(translated.value(), PCRE2_UTF | PCRE2_UCP |
                                                 PCRE2_NO_AUTO_CAPTURE |
                                                 PCRE2_MATCH_INVALID_UTF);
  }

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

  const char *const pattern_data{pattern.empty() ? "" : pattern.data()};

  const std::regex PREFIX_REGEX{R"(^\^([a-zA-Z0-9-_/@]+)(\.\*)?)"};
  std::cmatch matches_prefix;
  if (std::regex_match(pattern_data, pattern_data + pattern.size(),
                       matches_prefix, PREFIX_REGEX)) {
    return RegexTypePrefix{matches_prefix[1].str()};
  }

  const std::regex RANGE_REGEX{R"(^\^\.\{(\d+),(\d+)\}\$$)"};
  std::cmatch matches_range;
  if (std::regex_match(pattern_data, pattern_data + pattern.size(),
                       matches_range, RANGE_REGEX)) {
    const auto minimum_string = matches_range[1].str();
    const auto maximum_string = matches_range[2].str();
    std::size_t minimum{};
    std::size_t maximum{};
    const auto minimum_result =
        std::from_chars(minimum_string.data(),
                        minimum_string.data() + minimum_string.size(), minimum);
    const auto maximum_result =
        std::from_chars(maximum_string.data(),
                        maximum_string.data() + maximum_string.size(), maximum);
    if (minimum_result.ec != std::errc{} || maximum_result.ec != std::errc{}) {
      return std::nullopt;
    }

    // ECMA-262 defines "numbers out of order in {} quantifier" as a
    // SyntaxError, so such a pattern is not a valid regular expression
    if (minimum > maximum) {
      return std::nullopt;
    }

    return RegexTypeRange{minimum, maximum};
  }

  const auto translated{translate_permissive(std::string{pattern})};
  if (!translated.transformed.has_value()) {
    return std::nullopt;
  }

  // Capturing groups are kept enabled so that ECMA-262 numbered
  // backreferences like (a)\1 compile and match, at a small tracking cost
  return compile_pcre2(translated.transformed.value(),
                       PCRE2_UTF | PCRE2_UCP | PCRE2_DOTALL |
                           PCRE2_DOLLAR_ENDONLY | PCRE2_NEVER_BACKSLASH_C |
                           PCRE2_MATCH_INVALID_UTF | PCRE2_ALLOW_EMPTY_CLASS);
}

auto matches(const Regex &regex, const std::string_view value) -> bool {
  switch (static_cast<RegexIndex>(regex.index())) {
    case RegexIndex::Prefix:
      return value.starts_with(*std::get_if<RegexTypePrefix>(&regex));
    case RegexIndex::NonEmpty:
      return !value.empty();
    case RegexIndex::Range: {
      // ECMA-262 "." matches a single code point, not a single byte, so the
      // bounds are compared against the number of code points
      const RegexTypeRange *range{std::get_if<RegexTypeRange>(&regex)};
      return utf8_codepoint_within(value, range->first, range->second);
    }
    case RegexIndex::PCRE2: {
      const RegexTypePCRE2 *pcre2_regex{std::get_if<RegexTypePCRE2>(&regex)};
      auto *pcre2_code_ptr{static_cast<pcre2_code *>(pcre2_regex->code.get())};
      // Allocated once per thread and reused across calls to avoid
      // allocating every time. It is intentionally never freed, as
      // releasing it on thread exit would make matching unsafe during the
      // destruction of other objects with thread storage duration
      thread_local pcre2_match_data *match_data{
          pcre2_match_data_create(1, nullptr)};
      // A null context selects the built-in default limits, which guarantee
      // that pathological patterns applied to adversarial inputs terminate.
      // Note that exhausting any matching resource, like the machine stack
      // of the just-in-time fast path on long values, counts as a failure
      const int match_result{pcre2_match(
          pcre2_code_ptr, reinterpret_cast<PCRE2_SPTR>(value.data()),
          value.size(), 0, PCRE2_NO_UTF_CHECK, match_data, nullptr)};
      return match_result >= 0;
    }
    case RegexIndex::Noop:
      return true;
  }

  std::unreachable();
}

auto matches_if_valid(const std::string_view pattern,
                      const std::string_view value, const RegexDialect dialect)
    -> bool {
  const auto regex{to_regex(pattern, dialect)};
  return regex.has_value() && matches(regex.value(), value);
}

auto is_regex_ecma(const std::string_view pattern) -> bool {
  const auto translated{translate_permissive(std::string{pattern})};
  if (!translated.ecma_valid || !translated.transformed.has_value()) {
    return false;
  }

  if (compiles_with_ecma_options(translated.transformed.value())) {
    return true;
  }

  // ECMA-262 places no width restriction on lookbehind assertions, while
  // PCRE2 only accepts lookbehinds whose maximum width is bounded. As
  // lookbehind and lookahead bodies follow the same grammar, the syntax
  // check is retried with every lookbehind turned into a lookahead
  const auto rewritten{
      lookbehinds_as_lookaheads(translated.transformed.value())};
  return rewritten != translated.transformed.value() &&
         compiles_with_ecma_options(rewritten);
}

} // namespace sourcemeta::core
