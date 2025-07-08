#include <uriparser/Uri.h>

#include <sourcemeta/core/uri_escape.h>

#include <algorithm> // std::copy
#include <array>     // std::array
#include <cassert>   // assert
#include <cctype>    // std::isalnum
#include <istream>   // std::istream
#include <ostream>   // std::ostream
#include <sstream>   // std::ostringstream
#include <string>    // std::string

namespace sourcemeta::core {

auto uri_escape(const char character, std::ostream &output,
                const URIEscapeMode mode) -> void {
  const std::array<char, 16> HEX = {{'0', '1', '2', '3', '4', '5', '6', '7',
                                     '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'}};

  // unreserved = ALPHA / DIGIT / "-" / "." / "_" / "~"
  // See https://www.rfc-editor.org/rfc/rfc3986#appendix-A
  if (std::isalnum(character)) {
    output.put(character);

    // sub-delims = "!" / "$" / "&" / "'" / "(" / ")" / "*" / "+" / "," / ";" /
    // "=" See https://www.rfc-editor.org/rfc/rfc3986#appendix-A
  } else if (mode == URIEscapeMode::SkipSubDelims) {
    switch (character) {
      case '!':
      case '$':
      case '&':
      case '\'':
      case '(':
      case ')':
      case '*':
      case '+':
      case ',':
      case ';':
      case '=':

      // unreserved = ALPHA / DIGIT / "-" / "." / "_" / "~"
      // See https://www.rfc-editor.org/rfc/rfc3986#appendix-A
      case '-':
      case '_':
      case '.':
      case '~':
        output.put(character);
        break;
      default:
        output.put('%');
        output.put(HEX[static_cast<unsigned char>(character) >> 4]);
        output.put(HEX[static_cast<unsigned char>(character) & 0x0F]);
        break;
    }
  } else {
    assert(mode == URIEscapeMode::SkipUnreserved);
    // unreserved = ALPHA / DIGIT / "-" / "." / "_" / "~"
    // See https://www.rfc-editor.org/rfc/rfc3986#appendix-A
    switch (character) {
      case '-':
      case '_':
      case '.':
      case '~':
        output.put(character);
        break;
      default:
        output.put('%');
        output.put(HEX[static_cast<unsigned char>(character) >> 4]);
        output.put(HEX[static_cast<unsigned char>(character) & 0x0F]);
        break;
    }
  }
}

auto uri_escape(std::istream &input, std::ostream &output,
                const URIEscapeMode mode) -> void {
  char character = 0;
  while (input.get(character)) {
    uri_escape(character, output, mode);
  }
}

// TODO: Not very efficient. Can be better if we implement it from scratch
auto uri_unescape(std::istream &input, std::ostream &output) -> void {
  std::ostringstream input_stream;
  while (!input.eof()) {
    input_stream.put(static_cast<std::ostringstream::char_type>(input.get()));
  }

  const std::string input_string{input_stream.str()};
  auto const buffer = new std::string::value_type[input_string.size() + 1];
  try {
    std::ranges::copy(input_string, buffer);
  } catch (...) {
    delete[] buffer;
    throw;
  }

  buffer[input_string.size()] = '\0';
  const std::string::value_type *const new_end =
      uriUnescapeInPlaceExA(buffer, URI_FALSE, URI_BR_DONT_TOUCH);

  try {
    output.write(buffer, new_end - buffer - 1);
    delete[] buffer;
  } catch (...) {
    delete[] buffer;
    throw;
  }
}

} // namespace sourcemeta::core
