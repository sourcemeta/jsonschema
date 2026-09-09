#include <sourcemeta/core/terminal.h>

#include "terminal_ansi_escapes.h"
#include "terminal_internal.h"

#include <array>       // std::array
#include <atomic>      // std::atomic, std::memory_order_relaxed
#include <cassert>     // assert
#include <cstddef>     // std::size_t
#include <ostream>     // std::ostream
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::to_underlying

namespace {

// Indices match TerminalStream: Stdin = 0, Stdout = 1, Stderr = 2
std::array<std::atomic<sourcemeta::core::TerminalColorPolicy>, 3>
    stream_policies{{sourcemeta::core::TerminalColorPolicy::WhenInteractive,
                     sourcemeta::core::TerminalColorPolicy::WhenInteractive,
                     sourcemeta::core::TerminalColorPolicy::WhenInteractive}};

constexpr auto stream_index(sourcemeta::core::TerminalStream stream) noexcept
    -> std::size_t {
  return static_cast<std::size_t>(std::to_underlying(stream));
}

} // namespace

namespace sourcemeta::core {

auto terminal_is_interactive(TerminalStream stream) noexcept -> bool {
  return internal::is_interactive_stream(stream);
}

auto terminal_is_interactive(int file_descriptor) noexcept -> bool {
  return internal::is_interactive_fd(file_descriptor);
}

auto terminal_set_color_policy(TerminalColorPolicy policy) noexcept -> void {
  for (auto &stream_policy : stream_policies) {
    stream_policy.store(policy, std::memory_order_relaxed);
  }
}

auto terminal_set_color_policy(TerminalStream stream,
                               TerminalColorPolicy policy) noexcept -> void {
  stream_policies[stream_index(stream)].store(policy,
                                              std::memory_order_relaxed);
}

auto terminal_color_policy(TerminalStream stream) noexcept
    -> TerminalColorPolicy {
  return stream_policies[stream_index(stream)].load(std::memory_order_relaxed);
}

auto terminal_color_enabled(TerminalStream stream) noexcept -> bool {
  return terminal_color_policy(stream) != TerminalColorPolicy::Disabled &&
         terminal_is_interactive(stream);
}

auto terminal_sgr_reset() noexcept -> std::string_view {
  return internal::ESCAPE_RESET;
}

auto terminal_sgr_sequence(TerminalStyle style) noexcept -> std::string_view {
  assert(terminal_style_is_valid(style));

  if (style == TerminalStyle::None) {
    return {};
  }

  const bool has_bold{(style & TerminalStyle::Bold) != TerminalStyle::None};

  if ((style & TerminalStyle::Red) != TerminalStyle::None) {
    return has_bold ? internal::ESCAPE_BOLD_RED : internal::ESCAPE_RED;
  }
  if ((style & TerminalStyle::Green) != TerminalStyle::None) {
    return has_bold ? internal::ESCAPE_BOLD_GREEN : internal::ESCAPE_GREEN;
  }
  if ((style & TerminalStyle::Yellow) != TerminalStyle::None) {
    return has_bold ? internal::ESCAPE_BOLD_YELLOW : internal::ESCAPE_YELLOW;
  }
  if ((style & TerminalStyle::Blue) != TerminalStyle::None) {
    return has_bold ? internal::ESCAPE_BOLD_BLUE : internal::ESCAPE_BLUE;
  }
  if ((style & TerminalStyle::Cyan) != TerminalStyle::None) {
    return has_bold ? internal::ESCAPE_BOLD_CYAN : internal::ESCAPE_CYAN;
  }

  if (has_bold) {
    return internal::ESCAPE_BOLD;
  }

  return {};
}

auto terminal_paint(std::string_view text, TerminalStyle style, bool enabled)
    -> std::string {
  assert(terminal_style_is_valid(style));

  if (!enabled || style == TerminalStyle::None || text.empty()) {
    return std::string{text};
  }

  const std::string_view sequence{terminal_sgr_sequence(style)};
  if (sequence.empty()) {
    return std::string{text};
  }

  const std::string_view reset{terminal_sgr_reset()};
  std::string styled_text;
  styled_text.reserve(sequence.size() + text.size() + reset.size());
  styled_text.append(sequence);
  styled_text.append(text);
  styled_text.append(reset);
  return styled_text;
}

auto terminal_paint(TerminalStream stream, std::string_view text,
                    TerminalStyle style) -> std::string {
  assert(terminal_style_is_valid(style));

  const bool enabled{terminal_color_enabled(stream)};
  if (enabled && style != TerminalStyle::None && !text.empty()) {
    internal::enable_virtual_terminal_stream(stream);
  }
  return terminal_paint(text, style, enabled);
}

auto terminal_paint(std::ostream &output, std::string_view text,
                    TerminalStyle style, bool enabled) -> std::ostream & {
  assert(terminal_style_is_valid(style));

  if (!enabled || style == TerminalStyle::None || text.empty()) {
    output << text;
    return output;
  }

  const std::string_view sequence{terminal_sgr_sequence(style)};
  if (sequence.empty()) {
    output << text;
    return output;
  }

  output << sequence << text << terminal_sgr_reset();
  return output;
}

auto terminal_paint(std::ostream &output, TerminalStream stream,
                    std::string_view text, TerminalStyle style)
    -> std::ostream & {
  assert(terminal_style_is_valid(style));

  const bool enabled{terminal_color_enabled(stream)};
  if (enabled && style != TerminalStyle::None && !text.empty()) {
    internal::enable_virtual_terminal_stream(stream);
  }
  return terminal_paint(output, text, style, enabled);
}

} // namespace sourcemeta::core
