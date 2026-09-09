#ifndef SOURCEMETA_CORE_TERMINAL_H_
#define SOURCEMETA_CORE_TERMINAL_H_

#ifndef SOURCEMETA_CORE_TERMINAL_EXPORT
#include <sourcemeta/core/terminal_export.h>
#endif

#include <array>       // std::array
#include <cstdint>     // std::uint8_t
#include <iosfwd>      // std::ostream
#include <string>      // std::string
#include <string_view> // std::string_view
#include <type_traits> // std::underlying_type_t

/// @defgroup terminal Terminal
/// @brief Terminal detection, coloring policy, and ANSI styling utilities
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/terminal.h>
/// ```

namespace sourcemeta::core {

/// @ingroup terminal
///
/// Standard I/O streams defined by POSIX.1-2017 (<unistd.h>).
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/terminal.h>
/// #include <cassert>
///
/// const auto stream{sourcemeta::core::TerminalStream::Stdout};
/// assert(static_cast<int>(stream) == 1);
/// ```
///
/// @see https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/unistd.h.html
enum class TerminalStream : std::uint8_t {
  /// Standard input stream (POSIX.1-2017 STDIN_FILENO, 0).
  /// @see
  /// https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/unistd.h.html
  Stdin = 0,
  /// Standard output stream (POSIX.1-2017 STDOUT_FILENO, 1).
  /// @see
  /// https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/unistd.h.html
  Stdout = 1,
  /// Standard error stream (POSIX.1-2017 STDERR_FILENO, 2).
  /// @see
  /// https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/unistd.h.html
  Stderr = 2
};

/// @ingroup terminal
///
/// Color policy governing ANSI styling output.
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/terminal.h>
/// #include <cassert>
///
/// sourcemeta::core::terminal_set_color_policy(
///     sourcemeta::core::TerminalColorPolicy::Disabled);
/// assert(!sourcemeta::core::terminal_color_enabled(
///     sourcemeta::core::TerminalStream::Stdout));
/// ```
enum class TerminalColorPolicy : std::uint8_t {
  /// Color is applied when the destination stream is connected to an
  /// interactive terminal device.
  ///
  /// @see
  /// https://pubs.opengroup.org/onlinepubs/9699919799/functions/isatty.html
  WhenInteractive,
  /// Styling is unconditionally suppressed.
  Disabled
};

/// @ingroup terminal
///
/// Text styles and foreground colors defined by ECMA-48 (5th Edition, 1991),
/// Section 8.3.117 "SGR - SELECT GRAPHIC RENDITION" (also standardized as
/// ISO/IEC 6429).
///
/// Bitwise operators allow combining `TerminalStyle::Bold` with a foreground
/// color (for example `TerminalStyle::Bold | TerminalStyle::Red`). Only one
/// foreground color may be active at a time. If multiple foreground colors are
/// combined, the first matching color in declaration order (Red > Green >
/// Yellow > Blue > Cyan) takes precedence.
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/terminal.h>
/// #include <cassert>
///
/// const auto style{sourcemeta::core::TerminalStyle::Bold |
///                  sourcemeta::core::TerminalStyle::Green};
/// assert(sourcemeta::core::terminal_style_is_valid(style));
/// ```
///
/// @see
/// https://ecma-international.org/publications-and-standards/standards/ecma-48/
enum class TerminalStyle : std::uint8_t {
  /// Normal display / no style (plain text, ECMA-48 SGR parameter 0).
  None = 0,
  /// Bold or increased intensity (ECMA-48 SGR parameter 1).
  Bold = 1 << 0,
  /// Red foreground color (ECMA-48 SGR parameter 31).
  Red = 1 << 1,
  /// Green foreground color (ECMA-48 SGR parameter 32).
  Green = 1 << 2,
  /// Yellow foreground color (ECMA-48 SGR parameter 33).
  Yellow = 1 << 3,
  /// Blue foreground color (ECMA-48 SGR parameter 34).
  Blue = 1 << 4,
  /// Cyan foreground color (ECMA-48 SGR parameter 36).
  Cyan = 1 << 5
};

/// @ingroup terminal
///
/// Combine two styles using bitwise OR.
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/terminal.h>
/// #include <cassert>
///
/// const auto bold_red{sourcemeta::core::TerminalStyle::Bold |
///                     sourcemeta::core::TerminalStyle::Red};
/// assert((bold_red & sourcemeta::core::TerminalStyle::Bold) ==
///        sourcemeta::core::TerminalStyle::Bold);
/// ```
[[nodiscard]] constexpr auto operator|(TerminalStyle left,
                                       TerminalStyle right) noexcept
    -> TerminalStyle {
  using Underlying = std::underlying_type_t<TerminalStyle>;
  return static_cast<TerminalStyle>(static_cast<Underlying>(left) |
                                    static_cast<Underlying>(right));
}

/// @ingroup terminal
///
/// Intersect two styles using bitwise AND.
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/terminal.h>
/// #include <cassert>
///
/// const auto style{sourcemeta::core::TerminalStyle::Bold |
///                  sourcemeta::core::TerminalStyle::Blue};
/// assert((style & sourcemeta::core::TerminalStyle::Blue) ==
///        sourcemeta::core::TerminalStyle::Blue);
/// ```
[[nodiscard]] constexpr auto operator&(TerminalStyle left,
                                       TerminalStyle right) noexcept
    -> TerminalStyle {
  using Underlying = std::underlying_type_t<TerminalStyle>;
  return static_cast<TerminalStyle>(static_cast<Underlying>(left) &
                                    static_cast<Underlying>(right));
}

/// @ingroup terminal
///
/// Invert a style using bitwise NOT.
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/terminal.h>
/// #include <cassert>
///
/// const auto inverted{~sourcemeta::core::TerminalStyle::None};
/// assert((inverted & sourcemeta::core::TerminalStyle::Bold) ==
///        sourcemeta::core::TerminalStyle::Bold);
/// ```
[[nodiscard]] constexpr auto operator~(TerminalStyle value) noexcept
    -> TerminalStyle {
  using Underlying = std::underlying_type_t<TerminalStyle>;
  return static_cast<TerminalStyle>(~static_cast<Underlying>(value));
}

/// @ingroup terminal
///
/// XOR two styles.
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/terminal.h>
/// #include <cassert>
///
/// const auto result{sourcemeta::core::TerminalStyle::Bold ^
///                   sourcemeta::core::TerminalStyle::Bold};
/// assert(result == sourcemeta::core::TerminalStyle::None);
/// ```
[[nodiscard]] constexpr auto operator^(TerminalStyle left,
                                       TerminalStyle right) noexcept
    -> TerminalStyle {
  using Underlying = std::underlying_type_t<TerminalStyle>;
  return static_cast<TerminalStyle>(static_cast<Underlying>(left) ^
                                    static_cast<Underlying>(right));
}

/// @ingroup terminal
///
/// Check whether a terminal style configuration is valid and canonical.
///
/// A style is valid if it contains only defined bit flags and at most one
/// foreground color. For example:
///
/// ```cpp
/// #include <sourcemeta/core/terminal.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::terminal_style_is_valid(
///     sourcemeta::core::TerminalStyle::Bold |
///     sourcemeta::core::TerminalStyle::Red));
/// assert(!sourcemeta::core::terminal_style_is_valid(
///     sourcemeta::core::TerminalStyle::Red |
///     sourcemeta::core::TerminalStyle::Green));
/// ```
[[nodiscard]] constexpr auto
terminal_style_is_valid(TerminalStyle style) noexcept -> bool {
  using Underlying = std::underlying_type_t<TerminalStyle>;
  constexpr Underlying DEFINED_MASK{
      static_cast<Underlying>(TerminalStyle::Bold) |
      static_cast<Underlying>(TerminalStyle::Red) |
      static_cast<Underlying>(TerminalStyle::Green) |
      static_cast<Underlying>(TerminalStyle::Yellow) |
      static_cast<Underlying>(TerminalStyle::Blue) |
      static_cast<Underlying>(TerminalStyle::Cyan)};

  const auto raw{static_cast<Underlying>(style)};
  if ((raw & ~DEFINED_MASK) != 0) {
    return false;
  }

  constexpr std::array<TerminalStyle, 5> COLORS{
      {TerminalStyle::Red, TerminalStyle::Green, TerminalStyle::Yellow,
       TerminalStyle::Blue, TerminalStyle::Cyan}};

  int color_count{0};
  for (const auto color : COLORS) {
    if ((style & color) != TerminalStyle::None) {
      ++color_count;
      if (color_count > 1) {
        return false;
      }
    }
  }

  return true;
}

/// @ingroup terminal
///
/// Check whether the specified stream is connected to an interactive terminal,
/// according to POSIX.1-2017 isatty().
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/terminal.h>
///
/// const bool is_term{sourcemeta::core::terminal_is_interactive(
///     sourcemeta::core::TerminalStream::Stdout)};
/// ```
///
/// @see https://pubs.opengroup.org/onlinepubs/9699919799/functions/isatty.html
SOURCEMETA_CORE_TERMINAL_EXPORT
auto terminal_is_interactive(TerminalStream stream) noexcept -> bool;

/// @ingroup terminal
///
/// Check whether the specified file descriptor is connected to an interactive
/// terminal, according to POSIX.1-2017 isatty().
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/terminal.h>
/// #include <cassert>
///
/// assert(!sourcemeta::core::terminal_is_interactive(-1));
/// ```
///
/// @see https://pubs.opengroup.org/onlinepubs/9699919799/functions/isatty.html
SOURCEMETA_CORE_TERMINAL_EXPORT
auto terminal_is_interactive(int file_descriptor) noexcept -> bool;

/// @ingroup terminal
///
/// Set the global color policy across all streams.
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/terminal.h>
///
/// sourcemeta::core::terminal_set_color_policy(
///     sourcemeta::core::TerminalColorPolicy::Disabled);
/// ```
SOURCEMETA_CORE_TERMINAL_EXPORT
auto terminal_set_color_policy(TerminalColorPolicy policy) noexcept -> void;

/// @ingroup terminal
///
/// Set the color policy for a specific stream.
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/terminal.h>
///
/// sourcemeta::core::terminal_set_color_policy(
///     sourcemeta::core::TerminalStream::Stderr,
///     sourcemeta::core::TerminalColorPolicy::Disabled);
/// ```
SOURCEMETA_CORE_TERMINAL_EXPORT
auto terminal_set_color_policy(TerminalStream stream,
                               TerminalColorPolicy policy) noexcept -> void;

/// @ingroup terminal
///
/// Retrieve the current color policy for the specified stream.
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/terminal.h>
/// #include <cassert>
///
/// const auto policy{sourcemeta::core::terminal_color_policy()};
/// assert(policy == sourcemeta::core::TerminalColorPolicy::WhenInteractive ||
///        policy == sourcemeta::core::TerminalColorPolicy::Disabled);
/// ```
SOURCEMETA_CORE_TERMINAL_EXPORT
auto terminal_color_policy(
    TerminalStream stream = TerminalStream::Stdout) noexcept
    -> TerminalColorPolicy;

/// @ingroup terminal
///
/// Determine whether styling is enabled for the specified stream.
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/terminal.h>
///
/// const bool enabled{sourcemeta::core::terminal_color_enabled(
///     sourcemeta::core::TerminalStream::Stdout)};
/// ```
SOURCEMETA_CORE_TERMINAL_EXPORT
auto terminal_color_enabled(TerminalStream stream) noexcept -> bool;

/// @ingroup terminal
///
/// Return the ECMA-48 Select Graphic Rendition (SGR) control sequence for the
/// given style.
///
/// Returns an empty string view if `style` is `TerminalStyle::None`.
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/terminal.h>
/// #include <cassert>
///
/// const auto seq{sourcemeta::core::terminal_sgr_sequence(
///     sourcemeta::core::TerminalStyle::Bold)};
/// assert(seq == "\033[1m");
/// ```
///
/// @see
/// https://ecma-international.org/publications-and-standards/standards/ecma-48/
SOURCEMETA_CORE_TERMINAL_EXPORT
auto terminal_sgr_sequence(TerminalStyle style) noexcept -> std::string_view;

/// @ingroup terminal
///
/// Return the ECMA-48 Select Graphic Rendition (SGR) reset control sequence
/// (`"\033[0m"`).
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/terminal.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::terminal_sgr_reset() == "\033[0m");
/// ```
///
/// @see
/// https://ecma-international.org/publications-and-standards/standards/ecma-48/
SOURCEMETA_CORE_TERMINAL_EXPORT
auto terminal_sgr_reset() noexcept -> std::string_view;

/// @ingroup terminal
///
/// Wrap text in ECMA-48 Select Graphic Rendition (SGR) control sequences for
/// the given style if `enabled` is `true`.
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/terminal.h>
/// #include <cassert>
///
/// const std::string text{sourcemeta::core::terminal_paint(
///     "Alert", sourcemeta::core::TerminalStyle::Red, true)};
/// assert(text == "\033[31mAlert\033[0m");
/// ```
///
/// @see
/// https://ecma-international.org/publications-and-standards/standards/ecma-48/
SOURCEMETA_CORE_TERMINAL_EXPORT
auto terminal_paint(std::string_view text, TerminalStyle style,
                    bool enabled = true) -> std::string;

/// @ingroup terminal
///
/// Wrap text in ECMA-48 Select Graphic Rendition (SGR) control sequences for
/// the given stream destination.
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/terminal.h>
///
/// const std::string text{sourcemeta::core::terminal_paint(
///     sourcemeta::core::TerminalStream::Stderr, "Error",
///     sourcemeta::core::TerminalStyle::Red)};
/// ```
///
/// @see
/// https://ecma-international.org/publications-and-standards/standards/ecma-48/
SOURCEMETA_CORE_TERMINAL_EXPORT
auto terminal_paint(TerminalStream stream, std::string_view text,
                    TerminalStyle style) -> std::string;

/// @ingroup terminal
///
/// Stream styled text with ECMA-48 Select Graphic Rendition (SGR) control
/// sequences to an output stream without heap allocation.
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/terminal.h>
/// #include <iostream>
///
/// sourcemeta::core::terminal_paint(
///     std::cout, "Success", sourcemeta::core::TerminalStyle::Green, true);
/// ```
///
/// @see
/// https://ecma-international.org/publications-and-standards/standards/ecma-48/
SOURCEMETA_CORE_TERMINAL_EXPORT
auto terminal_paint(std::ostream &output, std::string_view text,
                    TerminalStyle style, bool enabled = true) -> std::ostream &;

/// @ingroup terminal
///
/// Stream styled text with ECMA-48 Select Graphic Rendition (SGR) control
/// sequences to an output stream for the given stream destination.
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/terminal.h>
/// #include <iostream>
///
/// sourcemeta::core::terminal_paint(
///     std::cerr, sourcemeta::core::TerminalStream::Stderr, "Fatal",
///     sourcemeta::core::TerminalStyle::Red);
/// ```
///
/// @see
/// https://ecma-international.org/publications-and-standards/standards/ecma-48/
SOURCEMETA_CORE_TERMINAL_EXPORT
auto terminal_paint(std::ostream &output, TerminalStream stream,
                    std::string_view text, TerminalStyle style)
    -> std::ostream &;

} // namespace sourcemeta::core

#endif
