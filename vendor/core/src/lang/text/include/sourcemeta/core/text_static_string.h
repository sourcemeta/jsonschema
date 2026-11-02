#ifndef SOURCEMETA_CORE_TEXT_STATIC_STRING_H_
#define SOURCEMETA_CORE_TEXT_STATIC_STRING_H_

#include <cstddef> // std::size_t

namespace sourcemeta::core {

/// @ingroup text
/// A null-terminated string that can only be built from a literal, and
/// therefore outlives every object that stores it.
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cstring>
/// #include <cassert>
///
/// const sourcemeta::core::StaticString message{"Something went wrong"};
/// assert(std::strcmp(message.data(), "Something went wrong") == 0);
/// ```
class StaticString {
public:
  /// Construct from a string literal.
  ///
  /// ```cpp
  /// #include <sourcemeta/core/text.h>
  ///
  /// const sourcemeta::core::StaticString message{"Something went wrong"};
  /// ```
  template <std::size_t Length>
  // Binding to the array rather than to a pointer is what rejects a message
  // that does not outlive the error, and the implicit conversion is what lets
  // a throw site read as a bare literal. It is not noexcept, as the throw
  // below is what rejects a bad argument, and nothing observes that, since a
  // consteval constructor never runs
  // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions,modernize-avoid-c-arrays)
  consteval StaticString(const char (&value)[Length]) : value_{value} {
    // A throw-expression is not a constant expression, which is how an array
    // that is not null-terminated becomes a compile-time error rather than a
    // read past its end. It can never run, as the only way to reach it is an
    // argument that does not compile
    if (value[Length - 1] != '\0') {
      throw "A static string must be null-terminated";
    }
  }

  /// Get the underlying null-terminated string.
  ///
  /// ```cpp
  /// #include <sourcemeta/core/text.h>
  /// #include <cstring>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::StaticString message{"Something went wrong"};
  /// assert(std::strlen(message.data()) == 20);
  /// ```
  [[nodiscard]] constexpr auto data() const noexcept -> const char * {
    return this->value_;
  }

private:
  const char *value_;
};

} // namespace sourcemeta::core

#endif
