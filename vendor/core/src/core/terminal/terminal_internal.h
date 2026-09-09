#ifndef SOURCEMETA_CORE_TERMINAL_INTERNAL_H_
#define SOURCEMETA_CORE_TERMINAL_INTERNAL_H_

#include <sourcemeta/core/terminal.h>

namespace sourcemeta::core::internal {

auto is_interactive_stream(TerminalStream stream) noexcept -> bool;
auto is_interactive_fd(int file_descriptor) noexcept -> bool;
auto enable_virtual_terminal_stream(TerminalStream stream) noexcept -> void;

} // namespace sourcemeta::core::internal

#endif
