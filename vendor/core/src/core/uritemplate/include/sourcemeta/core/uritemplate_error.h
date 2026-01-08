#ifndef SOURCEMETA_CORE_URITEMPLATE_ERROR_H_
#define SOURCEMETA_CORE_URITEMPLATE_ERROR_H_

#ifndef SOURCEMETA_CORE_URITEMPLATE_EXPORT
#include <sourcemeta/core/uritemplate_export.h>
#endif

#include <cstdint>     // std::uint64_t
#include <exception>   // std::exception
#include <filesystem>  // std::filesystem::path
#include <stdexcept>   // std::runtime_error
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace sourcemeta::core {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup uritemplate
/// An error that represents a URI Template parsing failure
class SOURCEMETA_CORE_URITEMPLATE_EXPORT URITemplateParseError
    : public std::exception {
public:
  URITemplateParseError(const std::uint64_t column) : column_{column} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "The input is not a valid URI Template";
  }

  /// Get the column number of the error
  [[nodiscard]] auto column() const noexcept -> std::uint64_t {
    return this->column_;
  }

private:
  std::uint64_t column_;
};

/// @ingroup uritemplate
/// An error that represents a URI Template expansion failure
class SOURCEMETA_CORE_URITEMPLATE_EXPORT URITemplateExpansionError
    : public std::runtime_error {
public:
  URITemplateExpansionError(const std::string &message)
      : std::runtime_error{message} {}
};

/// @ingroup uritemplate
/// An error that represents a variable name mismatch when adding routes
class SOURCEMETA_CORE_URITEMPLATE_EXPORT URITemplateRouterVariableMismatchError
    : public std::exception {
public:
  URITemplateRouterVariableMismatchError(const std::string_view left,
                                         const std::string_view right)
      : left_{left}, right_{right} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "Variable name mismatch when adding route";
  }

  /// Get the existing variable name
  [[nodiscard]] auto left() const noexcept -> const std::string & {
    return this->left_;
  }

  /// Get the conflicting variable name
  [[nodiscard]] auto right() const noexcept -> const std::string & {
    return this->right_;
  }

private:
  std::string left_;
  std::string right_;
};

/// @ingroup uritemplate
/// An error for invalid segments when adding routes
class SOURCEMETA_CORE_URITEMPLATE_EXPORT URITemplateRouterInvalidSegmentError
    : public std::exception {
public:
  URITemplateRouterInvalidSegmentError(const char *message,
                                       const std::string_view segment)
      : message_{message}, segment_{segment} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

  /// Get the offending segment
  [[nodiscard]] auto segment() const noexcept -> const std::string & {
    return this->segment_;
  }

private:
  const char *message_;
  std::string segment_;
};

/// @ingroup uritemplate
/// An error that represents a failure to save the router to disk
class SOURCEMETA_CORE_URITEMPLATE_EXPORT URITemplateRouterSaveError
    : public std::exception {
public:
  URITemplateRouterSaveError(std::filesystem::path path, const char *message)
      : path_{std::move(path)}, message_{message} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

private:
  std::filesystem::path path_;
  const char *message_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::core

#endif
