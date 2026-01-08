#ifndef SOURCEMETA_CORE_URITEMPLATE_H_
#define SOURCEMETA_CORE_URITEMPLATE_H_

#ifndef SOURCEMETA_CORE_URITEMPLATE_EXPORT
#include <sourcemeta/core/uritemplate_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/uritemplate_error.h>
#include <sourcemeta/core/uritemplate_router.h>
#include <sourcemeta/core/uritemplate_token.h>
// NOLINTEND(misc-include-cleaner)

#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint64_t
#include <functional>  // std::function
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view
#include <tuple>       // std::tuple
#include <type_traits> // std::void_t
#include <vector>      // std::vector

/// @defgroup uritemplate URI Template
/// @brief A strict RFC 6570 URI Template implementation.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/uritemplate.h>
/// ```

namespace sourcemeta::core {

/// @ingroup uritemplate
/// The return type for URI Template variable callbacks (value, key?, has_more)
using URITemplateValue = std::optional<
    std::tuple<std::string_view, std::optional<std::string_view>, bool>>;

/// @ingroup uritemplate
/// The result of parsing a token: the token and how many characters were
/// consumed
using URITemplateParseResult =
    std::optional<std::pair<URITemplateToken, std::size_t>>;

/// @ingroup uritemplate
/// A parsed URI Template per RFC 6570. This class behaves like a view. The
/// source string must outlive the template
class SOURCEMETA_CORE_URITEMPLATE_EXPORT URITemplate {
public:
  /// Parse a URI Template from a string view. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uritemplate.h>
  ///
  /// const std::string source{"http://example.com/~{username}/"};
  /// const sourcemeta::core::URITemplate uri_template{source};
  /// ```
  URITemplate(const std::string_view source);

  /// Get the number of tokens in the template
  [[nodiscard]] auto size() const noexcept -> std::uint64_t;

  /// Check if the template is empty
  [[nodiscard]] auto empty() const noexcept -> bool;

  /// Get the token at the given index
  [[nodiscard]] auto at(std::size_t index) const & -> const URITemplateToken &;

  /// Get the token at the given index (move overload)
  [[nodiscard]] auto at(std::size_t index) && -> URITemplateToken;

  /// Iterator to the beginning of the tokens
  [[nodiscard]] auto begin() const noexcept
      -> std::vector<URITemplateToken>::const_iterator;

  /// Iterator to the end of the tokens
  [[nodiscard]] auto end() const noexcept
      -> std::vector<URITemplateToken>::const_iterator;

  /// Expand the template by looking up variable values via a callback.
  /// The callback is called repeatedly for composite values
  [[nodiscard]] auto expand(
      const std::function<URITemplateValue(std::string_view)> &callback) const
      -> std::string;

  /// Expand the template using an associative container (string values only)
  template <typename Container,
            typename = std::void_t<typename Container::key_type>>
  [[nodiscard]] auto expand(const Container &variables) const -> std::string {
    return this->expand([&variables](
                            const std::string_view name) -> URITemplateValue {
      const auto iterator{variables.find(typename Container::key_type{name})};
      if (iterator == variables.end()) {
        return std::nullopt;
      } else {
        return std::make_tuple(std::string_view{iterator->second}, std::nullopt,
                               false);
      }
    });
  }

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  std::vector<URITemplateToken> tokens_;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::core

#endif
