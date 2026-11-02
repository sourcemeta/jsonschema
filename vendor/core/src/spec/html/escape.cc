#include <sourcemeta/core/html_escape.h>

#include <cstddef> // std::size_t
#include <string>  // std::string

namespace sourcemeta::core {

auto html_escape(std::string &text) -> void {
  const std::size_t original_size{text.size()};

  // First pass: count how much space we need
  std::size_t required_size{0};
  for (std::string::size_type position{0}; position < text.size();
       position += 1) {
    // The no-break space is replaced by its named entity, so its two UTF-8
    // bytes become six output bytes (HTML Living Standard "escaping a string"
    // step 2)
    if (static_cast<unsigned char>(text[position]) == 0xC2 &&
        position + 1 < text.size() &&
        static_cast<unsigned char>(text[position + 1]) == 0xA0) {
      required_size += 6; // &nbsp;
      position += 1;
      continue;
    }

    switch (text[position]) {
      case '&':
        required_size += 5; // &amp;
        break;
      case '<':
      case '>':
        required_size += 4; // &lt; or &gt;
        break;
      case '"':
        required_size += 6; // &quot;
        break;
      case '\'':
        required_size += 5; // &#39;
        break;
      default:
        required_size += 1;
    }
  }

  // If no escaping needed, return early
  if (required_size == original_size) {
    return;
  }

  // Write escaped characters backwards to avoid overwriting unprocessed data
  text.resize_and_overwrite(
      required_size,
      [original_size](char *buffer, std::size_t count) -> std::size_t {
        auto read_position = original_size;
        auto write_position = count;

        while (read_position > 0) {
          --read_position;

          // The no-break space (its two trailing UTF-8 bytes seen back to
          // front) is replaced by its named entity (HTML Living Standard
          // "escaping a string" step 2)
          if (static_cast<unsigned char>(buffer[read_position]) == 0xA0 &&
              read_position > 0 &&
              static_cast<unsigned char>(buffer[read_position - 1]) == 0xC2) {
            --read_position;
            write_position -= 6;
            buffer[write_position] = '&';
            buffer[write_position + 1] = 'n';
            buffer[write_position + 2] = 'b';
            buffer[write_position + 3] = 's';
            buffer[write_position + 4] = 'p';
            buffer[write_position + 5] = ';';
            continue;
          }

          const auto character = buffer[read_position];

          switch (character) {
            case '&':
              write_position -= 5;
              buffer[write_position] = '&';
              buffer[write_position + 1] = 'a';
              buffer[write_position + 2] = 'm';
              buffer[write_position + 3] = 'p';
              buffer[write_position + 4] = ';';
              break;
            case '<':
              write_position -= 4;
              buffer[write_position] = '&';
              buffer[write_position + 1] = 'l';
              buffer[write_position + 2] = 't';
              buffer[write_position + 3] = ';';
              break;
            case '>':
              write_position -= 4;
              buffer[write_position] = '&';
              buffer[write_position + 1] = 'g';
              buffer[write_position + 2] = 't';
              buffer[write_position + 3] = ';';
              break;
            case '"':
              write_position -= 6;
              buffer[write_position] = '&';
              buffer[write_position + 1] = 'q';
              buffer[write_position + 2] = 'u';
              buffer[write_position + 3] = 'o';
              buffer[write_position + 4] = 't';
              buffer[write_position + 5] = ';';
              break;
            case '\'':
              write_position -= 5;
              buffer[write_position] = '&';
              buffer[write_position + 1] = '#';
              buffer[write_position + 2] = '3';
              buffer[write_position + 3] = '9';
              buffer[write_position + 4] = ';';
              break;
            default:
              --write_position;
              buffer[write_position] = character;
          }
        }

        return count;
      });
}

} // namespace sourcemeta::core
