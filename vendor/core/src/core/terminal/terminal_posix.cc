#include "terminal_internal.h"

#include <unistd.h> // isatty

namespace sourcemeta::core::internal {

// Standards reference: POSIX.1-2017 (<unistd.h>), isatty()
// https://pubs.opengroup.org/onlinepubs/9699919799/functions/isatty.html
auto is_interactive_stream(TerminalStream stream) noexcept -> bool {
  return ::isatty(static_cast<int>(stream)) == 1;
}

// Standards reference: POSIX.1-2017 (<unistd.h>), isatty()
// https://pubs.opengroup.org/onlinepubs/9699919799/functions/isatty.html
auto is_interactive_fd(int file_descriptor) noexcept -> bool {
  if (file_descriptor < 0) {
    return false;
  }
  return ::isatty(file_descriptor) == 1;
}

auto enable_virtual_terminal_stream(TerminalStream) noexcept -> void {
  // POSIX terminals interpret ANSI escape sequences natively.
}

} // namespace sourcemeta::core::internal
