#ifndef SOURCEMETA_JSONSCHEMA_CLI_PRINT_H_
#define SOURCEMETA_JSONSCHEMA_CLI_PRINT_H_

#include <sourcemeta/core/terminal.h>

#include <cstdint>     // std::uint8_t
#include <cstdio>      // std::FILE, stdout, stderr, stdin
#include <format>      // std::format, std::format_string
#include <print>       // std::print, std::println
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::forward, std::unreachable

namespace sourcemeta::jsonschema {

enum class ValidationStatus : std::uint8_t { Pass, Fail };

inline auto to_file(sourcemeta::core::TerminalStream stream) noexcept
    -> std::FILE * {
  switch (stream) {
    case sourcemeta::core::TerminalStream::Stderr:
      return stderr;
    case sourcemeta::core::TerminalStream::Stdin:
      return stdin;
    case sourcemeta::core::TerminalStream::Stdout:
    default:
      return stdout;
  }
}

inline auto paint(std::string_view text, sourcemeta::core::TerminalStyle style,
                  sourcemeta::core::TerminalStream stream =
                      sourcemeta::core::TerminalStream::Stdout) -> std::string {
  return sourcemeta::core::terminal_paint(stream, text, style);
}

inline auto format_validation_status(ValidationStatus status) -> std::string {
  constexpr auto PASS_STYLE{sourcemeta::core::TerminalStyle::Bold |
                            sourcemeta::core::TerminalStyle::Green};
  constexpr auto FAIL_STYLE{sourcemeta::core::TerminalStyle::Bold |
                            sourcemeta::core::TerminalStyle::Red};

  if (sourcemeta::core::terminal_color_enabled(
          sourcemeta::core::TerminalStream::Stderr)) {
    switch (status) {
      case ValidationStatus::Pass:
        return paint("✓ ok:", PASS_STYLE,
                     sourcemeta::core::TerminalStream::Stderr);
      case ValidationStatus::Fail:
        return paint("✗ fail:", FAIL_STYLE,
                     sourcemeta::core::TerminalStream::Stderr);
    }
    std::unreachable();
  }

  switch (status) {
    case ValidationStatus::Pass:
      return "ok:";
    case ValidationStatus::Fail:
      return "fail:";
  }
  std::unreachable();
}

template <typename... Args>
inline auto println(sourcemeta::core::TerminalStyle style,
                    std::format_string<Args...> fmt, Args &&...args) -> void {
  const auto formatted = std::format(fmt, std::forward<Args>(args)...);
  std::println(
      "{}", paint(formatted, style, sourcemeta::core::TerminalStream::Stdout));
}

template <typename... Args>
inline auto println(sourcemeta::core::TerminalStream stream,
                    sourcemeta::core::TerminalStyle style,
                    std::format_string<Args...> fmt, Args &&...args) -> void {
  const auto formatted = std::format(fmt, std::forward<Args>(args)...);
  std::println(to_file(stream), "{}", paint(formatted, style, stream));
}

template <typename... Args>
inline auto print(sourcemeta::core::TerminalStyle style,
                  std::format_string<Args...> fmt, Args &&...args) -> void {
  const auto formatted = std::format(fmt, std::forward<Args>(args)...);
  std::print("{}",
             paint(formatted, style, sourcemeta::core::TerminalStream::Stdout));
}

template <typename... Args>
inline auto print(sourcemeta::core::TerminalStream stream,
                  sourcemeta::core::TerminalStyle style,
                  std::format_string<Args...> fmt, Args &&...args) -> void {
  const auto formatted = std::format(fmt, std::forward<Args>(args)...);
  std::print(to_file(stream), "{}", paint(formatted, style, stream));
}

} // namespace sourcemeta::jsonschema

#endif
