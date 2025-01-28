#include <sourcemeta/core/uri.h>
#include <uriparser/Uri.h>

#include <algorithm> // std::copy
#include <sstream>   // std::ostringstream

namespace sourcemeta::core {

// TODO: Not very efficient. Can be better if we implement it from scratch
auto URI::escape(std::istream &input, std::ostream &output) -> void {
  std::ostringstream input_stream;
  while (!input.eof()) {
    input_stream.put(static_cast<std::ostringstream::char_type>(input.get()));
  }

  const std::string input_string{input_stream.str()};
  const std::string::value_type *const input_c_str{input_string.c_str()};

  // Allocate 3 times more memory for the output as recommended in the docs
  // See
  // https://uriparser.github.io/doc/api/latest/Uri_8h.html#a5aa51f5859693dc901e8c2e23679cc5f
  std::string::value_type *buffer =
      new std::string::value_type[(input_string.size() * 3) + 1];
  std::string::value_type *const new_end =
      uriEscapeExA(input_c_str, input_c_str + input_string.size() - 1, buffer,
                   URI_FALSE, URI_FALSE);

  try {
    output.write(buffer, new_end - buffer);
    delete[] buffer;
  } catch (...) {
    delete[] buffer;
    throw;
  }
}

// TODO: Not very efficient. Can be better if we implement it from scratch
auto URI::unescape(std::istream &input, std::ostream &output) -> void {
  std::ostringstream input_stream;
  while (!input.eof()) {
    input_stream.put(static_cast<std::ostringstream::char_type>(input.get()));
  }

  const std::string input_string{input_stream.str()};
  std::string::value_type *const buffer =
      new std::string::value_type[input_string.size() + 1];
  try {
    std::copy(input_string.cbegin(), input_string.cend(), buffer);
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
