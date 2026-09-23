#ifndef SOURCEMETA_BLAZE_CONVERT_ERROR_H_
#define SOURCEMETA_BLAZE_CONVERT_ERROR_H_

#ifndef SOURCEMETA_BLAZE_CONVERT_EXPORT
#include <sourcemeta/blaze/convert_export.h>
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

/// @ingroup convert
/// An error that represents a dialect the conversion cannot move a schema from
class SOURCEMETA_BLAZE_CONVERT_EXPORT ConvertUnsupportedDialectError
    : public std::exception {
public:
  ConvertUnsupportedDialectError(const std::string_view identifier,
                                 sourcemeta::core::Pointer location)
      : identifier_{identifier}, location_{std::move(location)} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "The conversion does not support this dialect";
  }

  [[nodiscard]] auto identifier() const noexcept -> std::string_view {
    return this->identifier_;
  }

  [[nodiscard]] auto location() const noexcept
      -> const sourcemeta::core::Pointer & {
    return this->location_;
  }

private:
  std::string identifier_;
  sourcemeta::core::Pointer location_;
};

/// @ingroup convert
/// An error that represents a meta-schema that the conversion cannot move
class SOURCEMETA_BLAZE_CONVERT_EXPORT ConvertUnsupportedMetaschemaError
    : public std::exception {
public:
  ConvertUnsupportedMetaschemaError(const std::string_view identifier,
                                    sourcemeta::core::Pointer location)
      : identifier_{identifier}, location_{std::move(location)} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "The conversion does not support meta-schemas";
  }

  [[nodiscard]] auto identifier() const noexcept -> std::string_view {
    return this->identifier_;
  }

  [[nodiscard]] auto location() const noexcept
      -> const sourcemeta::core::Pointer & {
    return this->location_;
  }

private:
  std::string identifier_;
  sourcemeta::core::Pointer location_;
};

/// @ingroup convert
/// An error that represents a schema reference that does not point to a schema
class SOURCEMETA_BLAZE_CONVERT_EXPORT ConvertInvalidReferenceError
    : public std::exception {
public:
  ConvertInvalidReferenceError(const std::string_view identifier,
                               sourcemeta::core::Pointer location)
      : identifier_{identifier}, location_{std::move(location)} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "The reference does not point to a schema";
  }

  [[nodiscard]] auto identifier() const noexcept -> std::string_view {
    return this->identifier_;
  }

  [[nodiscard]] auto location() const noexcept
      -> const sourcemeta::core::Pointer & {
    return this->location_;
  }

private:
  std::string identifier_;
  sourcemeta::core::Pointer location_;
};

/// @ingroup convert
/// An error that represents a broken schema reference after conversion
class SOURCEMETA_BLAZE_CONVERT_EXPORT ConvertBrokenReferenceError
    : public std::exception {
public:
  ConvertBrokenReferenceError(const std::string_view identifier,
                              sourcemeta::core::Pointer location)
      : identifier_{identifier}, location_{std::move(location)} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "The reference broke after transformation";
  }

  [[nodiscard]] auto identifier() const noexcept -> std::string_view {
    return this->identifier_;
  }

  [[nodiscard]] auto location() const noexcept
      -> const sourcemeta::core::Pointer & {
    return this->location_;
  }

private:
  std::string identifier_;
  sourcemeta::core::Pointer location_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::blaze

#endif
