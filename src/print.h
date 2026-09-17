#ifndef SOURCEMETA_JSONSCHEMA_CLI_PRINT_H_
#define SOURCEMETA_JSONSCHEMA_CLI_PRINT_H_

#include <sourcemeta/core/terminal.h>

#include <cstdio>      // std::FILE, stdout, stderr, stdin
#include <format>      // std::format, std::format_string
#include <print>       // std::print, std::println
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::forward

namespace sourcemeta::jsonschema {

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
