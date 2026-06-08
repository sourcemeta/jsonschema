#include <sourcemeta/core/io_bytestream.h>

#include <cstddef> // std::byte
#include <cstdint> // std::uint8_t
#include <string>  // std::string

namespace {

auto bytes_to_string(std::initializer_list<std::uint8_t> bytes) -> std::string {
  std::string result;
  result.reserve(bytes.size());
  for (const auto byte : bytes) {
    result.push_back(static_cast<char>(byte));
  }
  return result;
}

} // namespace

namespace sourcemeta::core {

InputByteStream::InputByteStream(std::initializer_list<std::uint8_t> bytes)
    : std::istringstream{bytes_to_string(bytes)} {}

auto OutputByteStream::bytes() const -> std::vector<std::byte> {
  const auto contents{this->str()};
  std::vector<std::byte> result;
  result.reserve(contents.size());
  for (const auto character : contents) {
    result.push_back(
        static_cast<std::byte>(static_cast<std::uint8_t>(character)));
  }
  return result;
}

} // namespace sourcemeta::core
