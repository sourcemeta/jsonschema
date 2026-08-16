#ifndef SOURCEMETA_CORE_PROCESS_COMMAND_LINE_H_
#define SOURCEMETA_CORE_PROCESS_COMMAND_LINE_H_

#if defined(_WIN32) && !defined(__MSYS__) && !defined(__CYGWIN__) &&           \
    !defined(__MINGW32__) && !defined(__MINGW64__)

#include <cstddef>     // std::size_t
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

namespace {

// Quote a single argument for the inverse of CommandLineToArgvW, so that the
// child reconstructs the exact same argument vector
auto append_quoted_argument(std::string &command_line,
                            const std::string_view argument) -> void {
  const bool needs_quoting{argument.empty() ||
                           argument.find_first_of(" \t\"") !=
                               std::string_view::npos};

  if (!needs_quoting) {
    command_line.append(argument);
    return;
  }

  command_line.push_back('"');

  for (auto cursor = argument.cbegin();; ++cursor) {
    std::size_t backslash_count{0};
    while (cursor != argument.cend() && *cursor == '\\') {
      ++cursor;
      ++backslash_count;
    }

    if (cursor == argument.cend()) {
      command_line.append(backslash_count * 2, '\\');
      break;
    } else if (*cursor == '"') {
      command_line.append(backslash_count * 2 + 1, '\\');
      command_line.push_back('"');
    } else {
      command_line.append(backslash_count, '\\');
      command_line.push_back(*cursor);
    }
  }

  command_line.push_back('"');
}

} // namespace

} // namespace sourcemeta::core

#endif

#endif
