#include <sourcemeta/core/punycode.h>
#include <sourcemeta/core/punycode_error.h>

#include "utf8.h"

#include <algorithm> // std::ranges::sort
#include <cassert>   // assert
#include <cstdint>   // std::uint32_t, std::uint64_t
#include <limits>    // std::numeric_limits
#include <sstream>   // std::istringstream, std::ostringstream
#include <vector>    // std::vector

namespace sourcemeta::core {

// RFC 3492 Section 5: Bootstring parameters for Punycode
// See https://www.rfc-editor.org/rfc/rfc3492#section-5
static constexpr std::uint32_t BASE{36};
static constexpr std::uint32_t TMIN{1};
static constexpr std::uint32_t TMAX{26};
static constexpr std::uint32_t SKEW{38};
static constexpr std::uint32_t DAMP{700};
static constexpr std::uint32_t INITIAL_BIAS{72};
static constexpr std::uint32_t INITIAL_N{0x80};
static constexpr char DELIMITER{'-'};

// RFC 3492 Section 5: digit-value to code point
// See https://www.rfc-editor.org/rfc/rfc3492#section-5
static constexpr auto encode_digit(const std::uint32_t digit) -> char {
  assert(digit < BASE);
  if (digit < 26) {
    return static_cast<char>('a' + digit);
  }

  return static_cast<char>('0' + digit - 26);
}

// RFC 3492 Section 5: code point to digit-value
// See https://www.rfc-editor.org/rfc/rfc3492#section-5
static auto decode_digit(const char code_point) -> std::uint32_t {
  if (code_point >= 'a' && code_point <= 'z') {
    return static_cast<std::uint32_t>(code_point - 'a');
  } else if (code_point >= 'A' && code_point <= 'Z') {
    return static_cast<std::uint32_t>(code_point - 'A');
  } else if (code_point >= '0' && code_point <= '9') {
    return static_cast<std::uint32_t>(code_point - '0' + 26);
  }

  throw PunycodeError("Invalid digit");
}

// RFC 3492 Section 6.1: Bias adaptation function
// See https://www.rfc-editor.org/rfc/rfc3492#section-6.1
static auto adapt_bias(std::uint32_t delta,
                       const std::uint32_t number_of_points,
                       const bool first_time) -> std::uint32_t {
  delta = first_time ? delta / DAMP : delta / 2;
  delta += delta / number_of_points;
  std::uint32_t result{0};
  while (delta > ((BASE - TMIN) * TMAX) / 2) {
    delta /= (BASE - TMIN);
    result += BASE;
  }

  return result + (((BASE - TMIN + 1) * delta) / (delta + SKEW));
}

static constexpr auto compute_threshold(const std::uint32_t step,
                                        const std::uint32_t bias)
    -> std::uint32_t {
  if (step <= bias) {
    return TMIN;
  } else if (step >= bias + TMAX) {
    return TMAX;
  }

  return step - bias;
}

static constexpr auto is_basic(const char32_t code_point) -> bool {
  return code_point < INITIAL_N;
}

static auto punycode_encode(const std::u32string_view codepoints,
                            std::string &output) -> void {
  std::vector<char32_t> non_basic_sorted;
  non_basic_sorted.reserve(codepoints.size());

  for (const auto code_point : codepoints) {
    if (is_basic(code_point)) {
      output.push_back(static_cast<char>(code_point));
    } else {
      non_basic_sorted.push_back(code_point);
    }
  }

  const auto basic_count =
      static_cast<std::uint32_t>(codepoints.size() - non_basic_sorted.size());

  if (basic_count > 0) {
    output.push_back(DELIMITER);
  }

  if (non_basic_sorted.empty()) {
    return;
  }

  std::ranges::sort(non_basic_sorted);

  std::uint32_t current_code_point{INITIAL_N};
  std::uint32_t delta{0};
  std::uint32_t bias{INITIAL_BIAS};
  std::uint32_t handled_count{basic_count};
  std::size_t sorted_index{0};

  while (handled_count < codepoints.size()) {
    const char32_t minimum = non_basic_sorted[sorted_index];

    const auto delta_increment =
        static_cast<std::uint64_t>(minimum - current_code_point) *
        (handled_count + 1);
    if (delta_increment > std::numeric_limits<std::uint32_t>::max() -
                              static_cast<std::uint64_t>(delta)) {
      throw PunycodeError("Encode overflow");
    }

    delta += static_cast<std::uint32_t>(delta_increment);
    current_code_point = minimum;

    for (const auto code_point : codepoints) {
      if (code_point < current_code_point) {
        if (delta == std::numeric_limits<std::uint32_t>::max()) {
          throw PunycodeError("Encode overflow");
        }

        delta += 1;
      } else if (code_point == current_code_point) {
        std::uint32_t quotient{delta};

        for (std::uint32_t step{BASE};; step += BASE) {
          const std::uint32_t threshold = compute_threshold(step, bias);

          if (quotient < threshold) {
            break;
          }

          const std::uint32_t base_minus_threshold = BASE - threshold;
          output.push_back(encode_digit(
              threshold + ((quotient - threshold) % base_minus_threshold)));
          quotient = (quotient - threshold) / base_minus_threshold;
        }

        output.push_back(encode_digit(quotient));
        bias =
            adapt_bias(delta, handled_count + 1, handled_count == basic_count);
        delta = 0;
        handled_count += 1;
      }
    }

    while (sorted_index < non_basic_sorted.size() &&
           non_basic_sorted[sorted_index] == current_code_point) {
      sorted_index += 1;
    }

    delta += 1;
    current_code_point += 1;
  }
}

static auto punycode_decode(const std::string_view encoded,
                            std::u32string &decoded) -> void {
  std::uint32_t current_code_point{INITIAL_N};
  std::uint32_t insertion_index{0};
  std::uint32_t bias{INITIAL_BIAS};

  const auto delimiter_position = encoded.rfind(DELIMITER);
  std::size_t position{0};

  if (delimiter_position != std::string_view::npos) {
    decoded.reserve(encoded.size());
    for (std::size_t index = 0; index < delimiter_position; index += 1) {
      const auto code_point = static_cast<unsigned char>(encoded[index]);
      if (!is_basic(code_point)) {
        throw PunycodeError("Non-basic code point before delimiter");
      }

      decoded.push_back(code_point);
    }

    position = delimiter_position + 1;
  }

  while (position < encoded.size()) {
    const std::uint32_t previous_insertion_index{insertion_index};
    std::uint32_t weight_factor{1};

    for (std::uint32_t step{BASE};; step += BASE) {
      if (position >= encoded.size()) {
        throw PunycodeError("Unexpected end of input");
      }

      const std::uint32_t digit = decode_digit(encoded[position]);
      position += 1;

      if (digit >
          (std::numeric_limits<std::uint32_t>::max() - insertion_index) /
              weight_factor) {
        throw PunycodeError("Decode overflow");
      }

      insertion_index += digit * weight_factor;
      const std::uint32_t threshold = compute_threshold(step, bias);

      if (digit < threshold) {
        break;
      }

      const std::uint32_t base_minus_threshold = BASE - threshold;
      if (weight_factor >
          std::numeric_limits<std::uint32_t>::max() / base_minus_threshold) {
        throw PunycodeError("Decode overflow");
      }

      weight_factor *= base_minus_threshold;
    }

    const auto output_length = static_cast<std::uint32_t>(decoded.size()) + 1;
    bias = adapt_bias(insertion_index - previous_insertion_index, output_length,
                      previous_insertion_index == 0);

    const std::uint32_t increment = insertion_index / output_length;
    if (increment >
        std::numeric_limits<std::uint32_t>::max() - current_code_point) {
      throw PunycodeError("Decode overflow");
    }

    current_code_point += increment;
    insertion_index %= output_length;

    if (current_code_point < INITIAL_N) {
      throw PunycodeError("Decoded basic code point");
    }

    if (current_code_point > 0x10FFFF ||
        (current_code_point >= 0xD800 && current_code_point <= 0xDFFF)) {
      throw PunycodeError("Invalid code point");
    }

    decoded.insert(decoded.begin() +
                       static_cast<std::ptrdiff_t>(insertion_index),
                   static_cast<char32_t>(current_code_point));
    insertion_index += 1;
  }
}

auto utf32_to_punycode(std::u32string_view input) -> std::string {
  std::string result;
  punycode_encode(input, result);
  return result;
}

auto punycode_to_utf32(std::string_view input) -> std::u32string {
  std::u32string result;
  punycode_decode(input, result);
  return result;
}

auto utf8_to_punycode(std::istream &input, std::ostream &output) -> void {
  const auto codepoints = utf8_to_utf32(input);
  if (!codepoints.has_value()) {
    throw PunycodeError("Invalid UTF-8 input");
  }

  std::string result;
  punycode_encode(codepoints.value(), result);
  output << result;
}

auto punycode_to_utf8(std::istream &input, std::ostream &output) -> void {
  std::string encoded;
  char character{0};
  while (input.get(character)) {
    encoded.push_back(character);
  }

  std::u32string decoded;
  punycode_decode(encoded, decoded);
  utf32_to_utf8(decoded, output);
}

auto utf8_to_punycode(std::string_view input) -> std::string {
  std::istringstream input_stream{std::string{input}};
  const auto codepoints = utf8_to_utf32(input_stream);
  if (!codepoints.has_value()) {
    throw PunycodeError("Invalid UTF-8 input");
  }

  std::string result;
  punycode_encode(codepoints.value(), result);
  return result;
}

auto punycode_to_utf8(std::string_view input) -> std::string {
  std::u32string decoded;
  punycode_decode(input, decoded);
  std::ostringstream output_stream;
  utf32_to_utf8(decoded, output_stream);
  return output_stream.str();
}

} // namespace sourcemeta::core
