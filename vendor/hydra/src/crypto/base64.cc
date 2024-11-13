#include <sourcemeta/hydra/crypto.h>

template <typename CharT>
static auto base64_map_character(const CharT input, const std::size_t position,
                                 const std::size_t limit) -> CharT {
  static const CharT BASE64_DICTIONARY[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                           "abcdefghijklmnopqrstuvwxyz"
                                           "0123456789+/";
  return position > limit ? '='
                          : BASE64_DICTIONARY[static_cast<std::size_t>(input)];
}

namespace sourcemeta::hydra {

// Adapted from https://stackoverflow.com/a/31322410/1641422
auto base64_encode(std::string_view input, std::ostream &output) -> void {
  using CharT = std::ostream::char_type;
  const auto remainder{input.size() % 3};
  const std::size_t next_multiple_of_3{
      remainder == 0 ? input.size() : input.size() + (3 - remainder)};
  const std::size_t total_size{4 * (next_multiple_of_3 / 3)};
  const std::size_t limit{total_size - (next_multiple_of_3 - input.size()) - 1};
  std::size_t cursor = 0;
  for (std::size_t index = 0; index < total_size / 4; index++) {
    // Read a group of three bytes
    CharT triplet[3];
    triplet[0] = ((index * 3) + 0 < input.size()) ? input[(index * 3) + 0] : 0;
    triplet[1] = ((index * 3) + 1 < input.size()) ? input[(index * 3) + 1] : 0;
    triplet[2] = ((index * 3) + 2 < input.size()) ? input[(index * 3) + 2] : 0;

    // Transform into four base 64 characters
    CharT quad[4];
    quad[0] = static_cast<CharT>((triplet[0] & 0xfc) >> 2);
    quad[1] = static_cast<CharT>(((triplet[0] & 0x03) << 4) +
                                 ((triplet[1] & 0xf0) >> 4));
    quad[2] = static_cast<CharT>(((triplet[1] & 0x0f) << 2) +
                                 ((triplet[2] & 0xc0) >> 6));
    quad[3] = static_cast<CharT>(((triplet[2] & 0x3f) << 0));

    // Write resulting characters
    output.put(base64_map_character<CharT>(quad[0], cursor, limit));
    cursor += 1;
    output.put(base64_map_character<CharT>(quad[1], cursor, limit));
    cursor += 1;
    output.put(base64_map_character<CharT>(quad[2], cursor, limit));
    cursor += 1;
    output.put(base64_map_character<CharT>(quad[3], cursor, limit));
    cursor += 1;
  }
}

} // namespace sourcemeta::hydra
