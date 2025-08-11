#ifndef SOURCEMETA_CORE_OPTIONS_H_
#define SOURCEMETA_CORE_OPTIONS_H_

#ifndef SOURCEMETA_CORE_OPTIONS_EXPORT
#include <sourcemeta/core/options_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/options_error.h>
// NOLINTEND(misc-include-cleaner)

#include <cstddef>          // std::size_t
#include <initializer_list> // std::initializer_list
#include <memory>           // std::unique_ptr
#include <string>           // std::string
#include <string_view>      // std::string_view
#include <unordered_map>    // std::unordered_map
#include <unordered_set>    // std::unordered_set
#include <vector>           // std::vector

/// @defgroup options Options
/// @brief A simple and minimalistic UNIX-style command-line parsing library
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/options.h>
/// ```

namespace sourcemeta::core {

/// @ingroup options
/// Command-line option parsing modifiers
struct OptionsModifiers {
  /// Ignore the first N command-line arguments
  std::size_t skip{0};
};

/// @ingroup options
///
/// This class performs basic command-line argument parsing based on options,
/// flags, and aliases. For example:
///
/// ```cpp
/// #include <sourcemeta/core/options.h>
/// #include <cstdlib>
/// #include <iostream>
///
/// auto main(int argc, char *argv[]) -> int {
///   sourcemeta::core::Options app;
///   app.option("name", {"n"});
///   app.flag("shout", {"s"});
///   app.parse(argc, argv);
///
///   if (!app.contains("name")) {
///     std::cerr << "Missing name\n";
///     return EXIT_FAILURE;
///   }
///
///   std::cerr << "Hello, " << app.at("name").front();
///   if (app.contains("shout")) {
///     std::cerr << "!\n";
///   } else {
///     std::cerr << "\n";
///   }
///
///   return EXIT_SUCCESS;
/// }
/// ```
class SOURCEMETA_CORE_OPTIONS_EXPORT Options {
public:
  Options() = default;
  // Disallow copies given the unique pointers
  Options(const Options &) = delete;
  auto operator=(const Options &) -> Options & = delete;
  Options(Options &&) noexcept = default;
  auto operator=(Options &&) noexcept -> Options & = default;

  /// Declare a new option, which must take a value
  auto option(std::string &&name, std::initializer_list<std::string> aliases)
      -> void;

  /// Declare a new flag, which must not take a value
  auto flag(std::string &&name, std::initializer_list<std::string> aliases)
      -> void;

  /// Access the values (if any) set for an option or flag, by its main name
  [[nodiscard]] auto at(std::string_view name) const
      -> const std::vector<std::string_view> &;

  /// Check if an option or flag was set, by its main name
  [[nodiscard]] auto contains(std::string_view name) const -> bool;

  /// Access the positional arguments, if any
  [[nodiscard]] auto positional() const
      -> const std::vector<std::string_view> &;

  /// Parse program arguments given the declared options and flags
  auto parse(const int argc,
             // We want to be compatible with `main`s `argv`
             // NOLINTNEXTLINE(modernize-avoid-c-arrays)
             const char *const argv[], const OptionsModifiers options = {})
      -> void;

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif
  static constexpr std::string_view POSITIONAL_ARGUMENT_NAME{""};
  static const std::vector<std::string_view> EMPTY;

  std::vector<std::unique_ptr<std::string>> storage;
  std::unordered_map<std::string_view, std::string_view> aliases_;
  std::unordered_map<std::string_view, std::vector<std::string_view>> options_;
  std::unordered_set<std::string_view> flags;
#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif
};

} // namespace sourcemeta::core

#endif // SOURCEMETA_CORE_OPTIONS_H_
