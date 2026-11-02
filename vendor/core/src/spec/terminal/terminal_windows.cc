#include "terminal_internal.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <io.h>      // _isatty
#include <utility>   // std::unreachable
#include <windows.h> // GetStdHandle, GetConsoleMode, SetConsoleMode, STD_INPUT_HANDLE, STD_OUTPUT_HANDLE, STD_ERROR_HANDLE, ENABLE_VIRTUAL_TERMINAL_PROCESSING, DWORD, HANDLE, INVALID_HANDLE_VALUE

namespace sourcemeta::core::internal {

// Enables native processing of ECMA-48 / ANSI SGR escape sequences on Windows.
// Reference:
// https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
auto is_interactive_stream(TerminalStream stream) noexcept -> bool {
  DWORD standard_handle_id{STD_OUTPUT_HANDLE};
  switch (stream) {
    case TerminalStream::Stdin:
      standard_handle_id = STD_INPUT_HANDLE;
      break;
    case TerminalStream::Stdout:
      standard_handle_id = STD_OUTPUT_HANDLE;
      break;
    case TerminalStream::Stderr:
      standard_handle_id = STD_ERROR_HANDLE;
      break;
    default:
      std::unreachable();
  }

  const HANDLE handle{GetStdHandle(standard_handle_id)};
  if (handle == INVALID_HANDLE_VALUE || handle == nullptr) {
    return false;
  }

  DWORD console_mode{0};
  if (!GetConsoleMode(handle, &console_mode)) {
    return false;
  }

  if ((console_mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0) {
    return true;
  }

  // Check if virtual-terminal processing can be enabled without permanently
  // mutating the console mode
  if (SetConsoleMode(handle,
                     console_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
    SetConsoleMode(handle, console_mode);
    return true;
  }

  return false;
}

auto is_interactive_fd(int file_descriptor) noexcept -> bool {
  if (file_descriptor < 0) {
    return false;
  }
  return _isatty(file_descriptor) != 0;
}

auto enable_virtual_terminal_stream(TerminalStream stream) noexcept -> void {
  DWORD standard_handle_id{STD_OUTPUT_HANDLE};
  switch (stream) {
    case TerminalStream::Stdin:
      standard_handle_id = STD_INPUT_HANDLE;
      break;
    case TerminalStream::Stdout:
      standard_handle_id = STD_OUTPUT_HANDLE;
      break;
    case TerminalStream::Stderr:
      standard_handle_id = STD_ERROR_HANDLE;
      break;
    default:
      std::unreachable();
  }

  const HANDLE handle{GetStdHandle(standard_handle_id)};
  if (handle == INVALID_HANDLE_VALUE || handle == nullptr) {
    return;
  }

  DWORD console_mode{0};
  if (GetConsoleMode(handle, &console_mode)) {
    if ((console_mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) == 0) {
      SetConsoleMode(handle, console_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
  }
}

} // namespace sourcemeta::core::internal
