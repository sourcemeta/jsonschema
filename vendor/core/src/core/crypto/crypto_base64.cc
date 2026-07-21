#include <sourcemeta/core/crypto_base64.h>

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t, std::uint32_t
#include <optional>    // std::optional, std::nullopt
#include <ostream>     // std::ostream
#include <string>      // std::string
#include <string_view> // std::string_view

namespace {

// RFC 4648 Section 4, Table 1: The Base 64 Alphabet
constexpr std::string_view BASE64_ALPHABET{
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};

// RFC 4648 Section 5, Table 2: The "URL and Filename safe" Base 64 Alphabet
constexpr std::string_view BASE64URL_ALPHABET{
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"};

constexpr std::uint8_t INVALID_SEXTET{0xFF};

constexpr auto build_decode_table(const std::string_view alphabet) noexcept
    -> std::array<std::uint8_t, 256> {
  std::array<std::uint8_t, 256> table{};
  table.fill(INVALID_SEXTET);
  for (std::size_t index = 0; index < alphabet.size(); ++index) {
    table[static_cast<std::uint8_t>(alphabet[index])] =
        static_cast<std::uint8_t>(index);
  }
  return table;
}

constexpr std::array<std::uint8_t, 256> BASE64_DECODE_TABLE{
    build_decode_table(BASE64_ALPHABET)};
constexpr std::array<std::uint8_t, 256> BASE64URL_DECODE_TABLE{
    build_decode_table(BASE64URL_ALPHABET)};

template <typename Output>
auto encode(const std::string_view input, const std::string_view alphabet,
            const bool padding, Output &output) -> void {
  std::size_t index{0};
  while (index + 3 <= input.size()) {
    const std::uint32_t first{static_cast<std::uint8_t>(input[index])};
    const std::uint32_t second{static_cast<std::uint8_t>(input[index + 1])};
    const std::uint32_t third{static_cast<std::uint8_t>(input[index + 2])};
    output.push_back(alphabet[first >> 2u]);
    output.push_back(alphabet[((first & 0x03u) << 4u) | (second >> 4u)]);
    output.push_back(alphabet[((second & 0x0Fu) << 2u) | (third >> 6u)]);
    output.push_back(alphabet[third & 0x3Fu]);
    index += 3;
  }

  const auto remaining{input.size() - index};
  if (remaining == 1) {
    const std::uint32_t first{static_cast<std::uint8_t>(input[index])};
    output.push_back(alphabet[first >> 2u]);
    output.push_back(alphabet[(first & 0x03u) << 4u]);
    if (padding) {
      output.push_back('=');
      output.push_back('=');
    }
  } else if (remaining == 2) {
    const std::uint32_t first{static_cast<std::uint8_t>(input[index])};
    const std::uint32_t second{static_cast<std::uint8_t>(input[index + 1])};
    output.push_back(alphabet[first >> 2u]);
    output.push_back(alphabet[((first & 0x03u) << 4u) | (second >> 4u)]);
    output.push_back(alphabet[(second & 0x0Fu) << 2u]);
    if (padding) {
      output.push_back('=');
    }
  }
}

template <typename Output>
auto decode_into(const std::string_view input,
                 const std::array<std::uint8_t, 256> &table, const bool padding,
                 Output &output) -> bool {
  // Decoding appends to the output, so a failure after some bytes were written
  // rolls the output back to its original length rather than leaving a partial
  // decode in a reused buffer
  const auto base{output.size()};
  auto data{input};

  if (padding) {
    // RFC 4648 Section 4: "Special processing is performed if fewer than 24
    // bits are available at the end of the data being encoded. A full encoding
    // quantum is always completed at the end of a quantity", hence the padded
    // form must be a multiple of four characters
    if (data.size() % 4 != 0) {
      return false;
    }

    if (data.ends_with('=')) {
      data.remove_suffix(1);
      if (data.ends_with('=')) {
        data.remove_suffix(1);
      }
    }
  }

  if (data.size() % 4 == 1) {
    return false;
  }

  output.reserve(output.size() + ((data.size() / 4) * 3) + 2);

  std::size_t index{0};
  while (index + 4 <= data.size()) {
    const std::uint32_t first{table[static_cast<std::uint8_t>(data[index])]};
    const std::uint32_t second{
        table[static_cast<std::uint8_t>(data[index + 1])]};
    const std::uint32_t third{
        table[static_cast<std::uint8_t>(data[index + 2])]};
    const std::uint32_t fourth{
        table[static_cast<std::uint8_t>(data[index + 3])]};
    if (first == INVALID_SEXTET || second == INVALID_SEXTET ||
        third == INVALID_SEXTET || fourth == INVALID_SEXTET) {
      output.resize(base, '\0');
      return false;
    }

    const std::uint32_t group{(first << 18u) | (second << 12u) | (third << 6u) |
                              fourth};
    output.push_back(static_cast<char>((group >> 16u) & 0xFFu));
    output.push_back(static_cast<char>((group >> 8u) & 0xFFu));
    output.push_back(static_cast<char>(group & 0xFFu));
    index += 4;
  }

  // RFC 4648 Section 3.5: "Implementations MAY chose to reject the encoding
  // if the pad bits have not been set to zero". We reject so that every value
  // has exactly one accepted encoding
  const auto remaining{data.size() - index};
  if (remaining == 2) {
    const std::uint32_t first{table[static_cast<std::uint8_t>(data[index])]};
    const std::uint32_t second{
        table[static_cast<std::uint8_t>(data[index + 1])]};
    if (first == INVALID_SEXTET || second == INVALID_SEXTET ||
        (second & 0x0Fu) != 0) {
      output.resize(base, '\0');
      return false;
    }

    output.push_back(static_cast<char>((first << 2u) | (second >> 4u)));
  } else if (remaining == 3) {
    const std::uint32_t first{table[static_cast<std::uint8_t>(data[index])]};
    const std::uint32_t second{
        table[static_cast<std::uint8_t>(data[index + 1])]};
    const std::uint32_t third{
        table[static_cast<std::uint8_t>(data[index + 2])]};
    if (first == INVALID_SEXTET || second == INVALID_SEXTET ||
        third == INVALID_SEXTET || (third & 0x03u) != 0) {
      output.resize(base, '\0');
      return false;
    }

    output.push_back(static_cast<char>((first << 2u) | (second >> 4u)));
    output.push_back(
        static_cast<char>(((second & 0x0Fu) << 4u) | (third >> 2u)));
  }

  return true;
}

} // namespace

namespace sourcemeta::core {

auto base64_encode(const std::string_view input, std::ostream &output) -> void {
  output << base64_encode(input);
}

auto base64_encode(const std::string_view input) -> std::string {
  std::string result;
  result.reserve(((input.size() + 2) / 3) * 4);
  encode(input, BASE64_ALPHABET, true, result);
  return result;
}

auto base64_encode(const std::string_view input, SecureString &output) -> void {
  // The input is copied first so that growing the output cannot invalidate it
  // when the two alias the same storage
  const SecureString input_copy{input};
  output.reserve(output.size() + ((input_copy.size() + 2) / 3) * 4);
  encode(std::string_view{input_copy}, BASE64_ALPHABET, true, output);
}

auto base64_decode(const std::string_view input) -> std::optional<std::string> {
  std::string output;
  if (!decode_into(input, BASE64_DECODE_TABLE, true, output)) {
    return std::nullopt;
  }

  return output;
}

auto base64_decode(const std::string_view input, SecureString &output) -> bool {
  // The input is copied first so that growing the output cannot invalidate it
  // when the two alias the same storage
  const SecureString input_copy{input};
  return decode_into(std::string_view{input_copy}, BASE64_DECODE_TABLE, true,
                     output);
}

auto base64url_encode(const std::string_view input, std::ostream &output)
    -> void {
  output << base64url_encode(input);
}

auto base64url_encode(const std::string_view input) -> std::string {
  std::string result;
  result.reserve(((input.size() + 2) / 3) * 4);
  encode(input, BASE64URL_ALPHABET, false, result);
  return result;
}

auto base64url_encode(const std::string_view input, SecureString &output)
    -> void {
  // The input is copied first so that growing the output cannot invalidate it
  // when the two alias the same storage
  const SecureString input_copy{input};
  output.reserve(output.size() + ((input_copy.size() + 2) / 3) * 4);
  encode(std::string_view{input_copy}, BASE64URL_ALPHABET, false, output);
}

auto base64url_decode(const std::string_view input)
    -> std::optional<std::string> {
  std::string output;
  if (!decode_into(input, BASE64URL_DECODE_TABLE, false, output)) {
    return std::nullopt;
  }

  return output;
}

} // namespace sourcemeta::core
