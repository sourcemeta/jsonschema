#ifndef SOURCEMETA_BLAZE_FRAME_ERROR_H
#define SOURCEMETA_BLAZE_FRAME_ERROR_H

#ifndef SOURCEMETA_BLAZE_FRAME_EXPORT
#include <sourcemeta/blaze/frame_export.h>
#endif

#include <sourcemeta/core/jsonpointer.h>

#include <exception>   // std::exception
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace sourcemeta::blaze {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup frame
/// An error that represents a schema frame error
class SOURCEMETA_BLAZE_FRAME_EXPORT SchemaFrameError : public std::exception {
public:
  SchemaFrameError(const std::string_view identifier, const char *message)
      : identifier_{identifier}, message_{message} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

  [[nodiscard]] auto identifier() const noexcept -> std::string_view {
    return this->identifier_;
  }

private:
  std::string identifier_;
  const char *message_;
};

/// @ingroup frame
/// An error that represents a schema anchor collision error
class SOURCEMETA_BLAZE_FRAME_EXPORT SchemaAnchorCollisionError
    : public std::exception {
public:
  SchemaAnchorCollisionError(const std::string_view identifier,
                             sourcemeta::core::Pointer location,
                             sourcemeta::core::Pointer other)
      : identifier_{identifier}, location_(std::move(location)),
        other_(std::move(other)) {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "Schema anchor already exists";
  }

  [[nodiscard]] auto identifier() const noexcept -> std::string_view {
    return this->identifier_;
  }

  [[nodiscard]] auto location() const noexcept
      -> const sourcemeta::core::Pointer & {
    return this->location_;
  }

  [[nodiscard]] auto other() const noexcept
      -> const sourcemeta::core::Pointer & {
    return this->other_;
  }

private:
  std::string identifier_;
  sourcemeta::core::Pointer location_;
  sourcemeta::core::Pointer other_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::blaze

#endif
