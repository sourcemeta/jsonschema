#include <sourcemeta/core/html_escape.h>

#include <string> // std::string

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

static auto needs_escape(const std::string_view input) -> bool {
  for (std::string_view::size_type position{0}; position < input.size();
       position += 1) {
    // The no-break space also requires escaping (HTML Living Standard "escaping
    // a string" step 2)
    if (static_cast<unsigned char>(input[position]) == 0xC2 &&
        position + 1 < input.size() &&
        static_cast<unsigned char>(input[position + 1]) == 0xA0) {
      return true;
    }

    switch (input[position]) {
      case '&':
      case '<':
      case '>':
      case '"':
      case '\'':
        return true;
      default:
        break;
    }
  }

  return false;
}

auto html_escape_append(std::string &output, const std::string_view input)
    -> void {
  if (!needs_escape(input)) {
    output += input;
    return;
  }

  for (std::string_view::size_type position{0}; position < input.size();
       position += 1) {
    // The no-break space is replaced by its named entity (HTML Living Standard
    // "escaping a string" step 2)
    if (static_cast<unsigned char>(input[position]) == 0xC2 &&
        position + 1 < input.size() &&
        static_cast<unsigned char>(input[position + 1]) == 0xA0) {
      output += "&nbsp;";
      position += 1;
      continue;
    }

    switch (input[position]) {
      case '&':
        output += "&amp;";
        break;
      case '<':
        output += "&lt;";
        break;
      case '>':
        output += "&gt;";
        break;
      case '"':
        output += "&quot;";
        break;
      case '\'':
        output += "&#39;";
        break;
      default:
        output += input[position];
    }
  }
}

auto html_escape_append(HTMLBuffer &output, const std::string_view input)
    -> void {
  if (!needs_escape(input)) {
    output.append(input);
    return;
  }

  for (std::string_view::size_type position{0}; position < input.size();
       position += 1) {
    // The no-break space is replaced by its named entity (HTML Living Standard
    // "escaping a string" step 2)
    if (static_cast<unsigned char>(input[position]) == 0xC2 &&
        position + 1 < input.size() &&
        static_cast<unsigned char>(input[position + 1]) == 0xA0) {
      output.append("&nbsp;");
      position += 1;
      continue;
    }

    switch (input[position]) {
      case '&':
        output.append("&amp;");
        break;
      case '<':
        output.append("&lt;");
        break;
      case '>':
        output.append("&gt;");
        break;
      case '"':
        output.append("&quot;");
        break;
      case '\'':
        output.append("&#39;");
        break;
      default:
        output.append(input[position]);
    }
  }
}

} // namespace sourcemeta::core
