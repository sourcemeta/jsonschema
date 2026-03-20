#include <sourcemeta/core/html_escape.h>

#include <string> // std::string

namespace sourcemeta::core {

auto html_escape(std::string &text) -> void {
  std::size_t write_position{0};
  std::size_t original_size{text.size()};

  // First pass: count how much space we need
  std::size_t required_size{0};
  for (char character : text) {
    switch (character) {
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

  // Resize string to accommodate escaped characters
  text.resize(required_size);

  // Second pass: work backwards to avoid overwriting data
  std::size_t read_position{original_size};
  write_position = required_size;

  while (read_position > 0) {
    --read_position;
    char character = text[read_position];

    switch (character) {
      case '&':
        write_position -= 5;
        text[write_position] = '&';
        text[write_position + 1] = 'a';
        text[write_position + 2] = 'm';
        text[write_position + 3] = 'p';
        text[write_position + 4] = ';';
        break;
      case '<':
        write_position -= 4;
        text[write_position] = '&';
        text[write_position + 1] = 'l';
        text[write_position + 2] = 't';
        text[write_position + 3] = ';';
        break;
      case '>':
        write_position -= 4;
        text[write_position] = '&';
        text[write_position + 1] = 'g';
        text[write_position + 2] = 't';
        text[write_position + 3] = ';';
        break;
      case '"':
        write_position -= 6;
        text[write_position] = '&';
        text[write_position + 1] = 'q';
        text[write_position + 2] = 'u';
        text[write_position + 3] = 'o';
        text[write_position + 4] = 't';
        text[write_position + 5] = ';';
        break;
      case '\'':
        write_position -= 5;
        text[write_position] = '&';
        text[write_position + 1] = '#';
        text[write_position + 2] = '3';
        text[write_position + 3] = '9';
        text[write_position + 4] = ';';
        break;
      default:
        --write_position;
        text[write_position] = character;
    }
  }
}

static auto needs_escape(const std::string_view input) -> bool {
  for (const char character : input) {
    switch (character) {
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

  for (const char character : input) {
    switch (character) {
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
        output += character;
    }
  }
}

auto html_escape_append(HTMLBuffer &output, const std::string_view input)
    -> void {
  if (!needs_escape(input)) {
    output.append(input);
    return;
  }

  for (const char character : input) {
    switch (character) {
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
        output.append(character);
    }
  }
}

} // namespace sourcemeta::core
