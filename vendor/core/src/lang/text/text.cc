#include <sourcemeta/core/text.h>

#include <cctype>  // std::isalpha, std::toupper
#include <cstddef> // std::size_t

namespace sourcemeta::core {

auto to_title_case(std::string &value) -> void {
  std::size_t write{0};
  bool capitalize_next{true};
  bool pending_separator{false};
  for (const char character : value) {
    if (character == '_' || character == '-') {
      if (write > 0) {
        pending_separator = true;
      }
    } else {
      if (pending_separator) {
        value[write++] = ' ';
        pending_separator = false;
        capitalize_next = true;
      }
      if (capitalize_next) {
        value[write++] = static_cast<char>(
            std::toupper(static_cast<unsigned char>(character)));
        if (std::isalpha(static_cast<unsigned char>(character))) {
          capitalize_next = false;
        }
      } else {
        value[write++] = character;
      }
    }
  }
  value.resize(write);
}

} // namespace sourcemeta::core
