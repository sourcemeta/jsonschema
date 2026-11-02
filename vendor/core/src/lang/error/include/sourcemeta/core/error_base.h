#ifndef SOURCEMETA_CORE_ERROR_BASE_H_
#define SOURCEMETA_CORE_ERROR_BASE_H_

#ifndef SOURCEMETA_CORE_ERROR_EXPORT
#include <sourcemeta/core/error_export.h>
#endif

#include <sourcemeta/core/text_static_string.h>

#include <exception> // std::exception

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#define SOURCEMETA_CORE_ERROR_WARNINGS_PUSH                                    \
  __pragma(warning(push)) __pragma(warning(disable : 4251 4275))
#define SOURCEMETA_CORE_ERROR_WARNINGS_POP __pragma(warning(pop))
#else
#define SOURCEMETA_CORE_ERROR_WARNINGS_PUSH
#define SOURCEMETA_CORE_ERROR_WARNINGS_POP
#endif

namespace sourcemeta::core {

SOURCEMETA_CORE_ERROR_WARNINGS_PUSH

/// @ingroup error
/// The base class of every error this project raises. It cannot be constructed
/// on its own, and its message is fixed at compile time.
///
/// ```cpp
/// #include <sourcemeta/core/error.h>
/// #include <cassert>
/// #include <string>
///
/// class MyError : public sourcemeta::core::Error {
/// public:
///   MyError() : sourcemeta::core::Error{"Something went wrong"} {}
/// };
///
/// try {
///   throw MyError{};
/// } catch (const sourcemeta::core::Error &error) {
///   assert(error.what() == std::string{"Something went wrong"});
/// }
/// ```
class SOURCEMETA_CORE_ERROR_EXPORT Error : public std::exception {
public:
  [[nodiscard]] auto what() const noexcept -> const char * final;

  /// Get the message in the form another error can take it, which is how a
  /// handler raises a different error without restating the reason.
  ///
  /// ```cpp
  /// #include <sourcemeta/core/error.h>
  /// #include <cassert>
  /// #include <string>
  ///
  /// class MyError : public sourcemeta::core::Error {
  /// public:
  ///   MyError() : sourcemeta::core::Error{"Something went wrong"} {}
  /// };
  ///
  /// class MyFileError : public sourcemeta::core::Error {
  /// public:
  ///   MyFileError(const sourcemeta::core::StaticString message)
  ///       : sourcemeta::core::Error{message} {}
  /// };
  ///
  /// try {
  ///   throw MyError{};
  /// } catch (const sourcemeta::core::Error &error) {
  ///   const MyFileError wrapped{error.message()};
  ///   assert(wrapped.what() == std::string{"Something went wrong"});
  /// }
  /// ```
  [[nodiscard]] auto message() const noexcept -> StaticString {
    return this->message_;
  }

protected:
  /// Construct the error given its message.
  explicit Error(const StaticString message) noexcept : message_{message} {}

private:
  StaticString message_;
};

SOURCEMETA_CORE_ERROR_WARNINGS_POP

} // namespace sourcemeta::core

#endif
