#include <sourcemeta/core/semver.h>

#include <limits>   // std::numeric_limits
#include <optional> // std::optional, std::nullopt
#include <string>   // std::string, std::to_string

namespace {

auto is_digit(const char character) -> bool {
  return character >= '0' && character <= '9';
}

auto is_letter(const char character) -> bool {
  return (character >= 'A' && character <= 'Z') ||
         (character >= 'a' && character <= 'z');
}

auto is_identifier_character(const char character) -> bool {
  return is_digit(character) || is_letter(character) || character == '-';
}

constexpr auto UINT64_MAX_VALUE = std::numeric_limits<std::uint64_t>::max();
constexpr auto UINT64_MAX_DIV_10 = UINT64_MAX_VALUE / 10;
constexpr auto UINT64_MAX_MOD_10 = UINT64_MAX_VALUE % 10;

enum class NumericParseResult : std::uint8_t { success, invalid, overflow };

auto parse_numeric_identifier(const std::string_view input,
                              std::size_t &position, std::uint64_t &result)
    -> NumericParseResult {
  if (position >= input.size() || !is_digit(input[position])) {
    return NumericParseResult::invalid;
  }

  if (input[position] == '0' && position + 1 < input.size() &&
      is_digit(input[position + 1])) {
    return NumericParseResult::invalid;
  }

  std::uint64_t value = 0;
  while (position < input.size() && is_digit(input[position])) {
    const auto digit = static_cast<std::uint64_t>(input[position] - '0');
    if (value > UINT64_MAX_DIV_10 ||
        (value == UINT64_MAX_DIV_10 && digit > UINT64_MAX_MOD_10)) {
      return NumericParseResult::overflow;
    }

    value = value * 10 + digit;
    ++position;
  }

  result = value;
  return NumericParseResult::success;
}

auto validate_pre_release_identifier(const std::string_view identifier)
    -> bool {
  if (identifier.empty()) {
    return false;
  }

  bool has_non_digit = false;
  for (const auto character : identifier) {
    if (!is_identifier_character(character)) {
      return false;
    }

    if (!is_digit(character)) {
      has_non_digit = true;
    }
  }

  if (!has_non_digit && identifier.size() > 1 && identifier[0] == '0') {
    return false;
  }

  return true;
}

auto validate_build_identifier(const std::string_view identifier) -> bool {
  if (identifier.empty()) {
    return false;
  }

  for (const auto character : identifier) {
    if (!is_identifier_character(character)) {
      return false;
    }
  }

  return true;
}

template <auto validator>
auto validate_dot_separated(const std::string_view input) -> bool {
  if (input.empty()) {
    return false;
  }

  std::size_t start = 0;
  while (start <= input.size()) {
    auto dot_position = input.find('.', start);
    if (dot_position == std::string_view::npos) {
      dot_position = input.size();
    }

    if (!validator(input.substr(start, dot_position - start))) {
      return false;
    }

    start = dot_position + 1;
    if (dot_position == input.size()) {
      break;
    }
  }

  return true;
}

struct IdentifierInfo {
  bool is_numeric;
  bool overflowed;
  std::uint64_t numeric_value;
};

auto classify_identifier(const std::string_view identifier) noexcept
    -> IdentifierInfo {
  std::uint64_t value = 0;
  for (const auto character : identifier) {
    if (!is_digit(character)) {
      return {.is_numeric = false, .overflowed = false, .numeric_value = 0};
    }

    const auto digit = static_cast<std::uint64_t>(character - '0');
    if (value > UINT64_MAX_DIV_10 ||
        (value == UINT64_MAX_DIV_10 && digit > UINT64_MAX_MOD_10)) {
      return {.is_numeric = true, .overflowed = true, .numeric_value = 0};
    }

    value = value * 10 + digit;
  }

  return {.is_numeric = true, .overflowed = false, .numeric_value = value};
}

auto compare_pre_release(const std::string_view left,
                         const std::string_view right) noexcept -> int {
  if (left.empty() && right.empty()) {
    return 0;
  }

  if (left.empty()) {
    return 1;
  }

  if (right.empty()) {
    return -1;
  }

  std::size_t left_position = 0;
  std::size_t right_position = 0;

  while (left_position <= left.size() && right_position <= right.size()) {
    auto left_dot = left.find('.', left_position);
    if (left_dot == std::string_view::npos) {
      left_dot = left.size();
    }

    auto right_dot = right.find('.', right_position);
    if (right_dot == std::string_view::npos) {
      right_dot = right.size();
    }

    const std::string_view left_identifier{left.data() + left_position,
                                           left_dot - left_position};
    const std::string_view right_identifier{right.data() + right_position,
                                            right_dot - right_position};

    const auto left_info = classify_identifier(left_identifier);
    const auto right_info = classify_identifier(right_identifier);

    if (left_info.is_numeric && right_info.is_numeric) {
      if (left_info.overflowed || right_info.overflowed) {
        if (left_identifier.size() != right_identifier.size()) {
          return left_identifier.size() < right_identifier.size() ? -1 : 1;
        }

        if (left_identifier < right_identifier) {
          return -1;
        }

        if (left_identifier > right_identifier) {
          return 1;
        }
      } else {
        if (left_info.numeric_value < right_info.numeric_value) {
          return -1;
        }

        if (left_info.numeric_value > right_info.numeric_value) {
          return 1;
        }
      }
    } else if (left_info.is_numeric && !right_info.is_numeric) {
      return -1;
    } else if (!left_info.is_numeric && right_info.is_numeric) {
      return 1;
    } else {
      if (left_identifier < right_identifier) {
        return -1;
      }

      if (left_identifier > right_identifier) {
        return 1;
      }
    }

    left_position = left_dot + 1;
    right_position = right_dot + 1;

    if (left_dot == left.size() && right_dot == right.size()) {
      break;
    }

    if (left_dot == left.size()) {
      return -1;
    }

    if (right_dot == right.size()) {
      return 1;
    }
  }

  return 0;
}

template <bool should_throw, bool loose>
auto parse_semver(const std::string_view input, std::uint64_t &major,
                  std::uint64_t &minor, std::uint64_t &patch,
                  std::string_view &pre_release, std::string_view &build)
    -> bool {
  std::size_t position = 0;

  if constexpr (loose) {
    if (position < input.size() &&
        (input[position] == 'v' || input[position] == 'V')) {
      ++position;
    }
  }

  const auto major_result = parse_numeric_identifier(input, position, major);
  if (major_result == NumericParseResult::overflow) {
    if constexpr (should_throw) {
      throw sourcemeta::core::SemVerOverflowError(position + 1);
    }

    return false;
  }

  if (major_result == NumericParseResult::invalid) {
    if constexpr (should_throw) {
      throw sourcemeta::core::SemVerParseError(position + 1);
    }

    return false;
  }

  auto can_end_core = [&]() -> bool {
    if (position >= input.size() || input[position] == '-' ||
        input[position] == '+') {
      return loose;
    }

    return input[position] == '.';
  };

  if (!can_end_core()) {
    if constexpr (should_throw) {
      throw sourcemeta::core::SemVerParseError(position + 1);
    }

    return false;
  }

  if (position < input.size() && input[position] == '.') {
    ++position;

    const auto minor_result = parse_numeric_identifier(input, position, minor);
    if (minor_result == NumericParseResult::overflow) {
      if constexpr (should_throw) {
        throw sourcemeta::core::SemVerOverflowError(position + 1);
      }

      return false;
    }

    if (minor_result == NumericParseResult::invalid) {
      if constexpr (should_throw) {
        throw sourcemeta::core::SemVerParseError(position + 1);
      }

      return false;
    }

    if (!can_end_core()) {
      if constexpr (should_throw) {
        throw sourcemeta::core::SemVerParseError(position + 1);
      }

      return false;
    }

    if (position < input.size() && input[position] == '.') {
      ++position;

      const auto patch_result =
          parse_numeric_identifier(input, position, patch);
      if (patch_result == NumericParseResult::overflow) {
        if constexpr (should_throw) {
          throw sourcemeta::core::SemVerOverflowError(position + 1);
        }

        return false;
      }

      if (patch_result == NumericParseResult::invalid) {
        if constexpr (should_throw) {
          throw sourcemeta::core::SemVerParseError(position + 1);
        }

        return false;
      }
    }
  }

  if (position < input.size() && input[position] == '-') {
    ++position;
    const auto start = position;
    while (position < input.size() && input[position] != '+') {
      ++position;
    }

    pre_release = input.substr(start, position - start);
    if (!validate_dot_separated<validate_pre_release_identifier>(pre_release)) {
      if constexpr (should_throw) {
        throw sourcemeta::core::SemVerParseError(start + 1);
      }

      return false;
    }
  }

  if (position < input.size() && input[position] == '+') {
    ++position;
    const auto start = position;
    position = input.size();

    build = input.substr(start, position - start);
    if (!validate_dot_separated<validate_build_identifier>(build)) {
      if constexpr (should_throw) {
        throw sourcemeta::core::SemVerParseError(start + 1);
      }

      return false;
    }
  }

  if (position != input.size()) {
    if constexpr (should_throw) {
      throw sourcemeta::core::SemVerParseError(position + 1);
    }

    return false;
  }

  return true;
}

} // namespace

namespace sourcemeta::core {

SemVer::SemVer(const std::string_view input, const Mode mode) {
  if (mode == Mode::Loose) {
    parse_semver<true, true>(input, this->major_, this->minor_, this->patch_,
                             this->pre_release_, this->build_);
  } else {
    parse_semver<true, false>(input, this->major_, this->minor_, this->patch_,
                              this->pre_release_, this->build_);
  }
}

auto SemVer::from(const std::string_view input, const Mode mode) noexcept
    -> std::optional<SemVer> {
  SemVer result;
  bool success = false;

  if (mode == Mode::Loose) {
    success = parse_semver<false, true>(input, result.major_, result.minor_,
                                        result.patch_, result.pre_release_,
                                        result.build_);
  } else {
    success = parse_semver<false, false>(input, result.major_, result.minor_,
                                         result.patch_, result.pre_release_,
                                         result.build_);
  }

  if (success) {
    return result;
  }

  return std::nullopt;
}

auto SemVer::operator<(const SemVer &other) const noexcept -> bool {
  if (this->major_ != other.major_) {
    return this->major_ < other.major_;
  }

  if (this->minor_ != other.minor_) {
    return this->minor_ < other.minor_;
  }

  if (this->patch_ != other.patch_) {
    return this->patch_ < other.patch_;
  }

  return compare_pre_release(this->pre_release_, other.pre_release_) < 0;
}

auto SemVer::to_string() const -> std::string {
  std::string result = std::to_string(this->major_);
  result += '.';
  result += std::to_string(this->minor_);
  result += '.';
  result += std::to_string(this->patch_);
  if (!this->pre_release_.empty()) {
    result += '-';
    result.append(this->pre_release_.data(), this->pre_release_.size());
  }

  if (!this->build_.empty()) {
    result += '+';
    result.append(this->build_.data(), this->build_.size());
  }

  return result;
}

} // namespace sourcemeta::core
