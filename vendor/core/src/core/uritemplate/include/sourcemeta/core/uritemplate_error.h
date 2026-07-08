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
  /// Construct an error given the column number where parsing failed
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
  /// Construct an error with a descriptive message
  URITemplateExpansionError(const std::string &message)
      : std::runtime_error{message} {}
};

/// @ingroup uritemplate
/// An error that represents a variable name mismatch when adding routes
class SOURCEMETA_CORE_URITEMPLATE_EXPORT URITemplateRouterVariableMismatchError
    : public std::exception {
public:
  /// Construct an error given the existing and conflicting variable names
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
  /// Construct an error given a descriptive message and the offending segment
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
/// An error that represents an operation identifier that does not match the
/// permitted format
class SOURCEMETA_CORE_URITEMPLATE_EXPORT
    URITemplateRouterInvalidOperationIdError : public std::exception {
public:
  /// Construct an error given the offending operation identifier
  URITemplateRouterInvalidOperationIdError(const std::string_view operation_id)
      : operation_id_{operation_id} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "Invalid operation identifier";
  }

  /// Get the offending operation identifier
  [[nodiscard]] auto operation_id() const noexcept -> const std::string & {
    return this->operation_id_;
  }

private:
  std::string operation_id_;
};

/// @ingroup uritemplate
/// An error that represents an operation identifier that conflicts with a
/// previously registered route
class SOURCEMETA_CORE_URITEMPLATE_EXPORT
    URITemplateRouterDuplicateOperationIdError : public std::exception {
public:
  /// Construct an error given the conflicting operation identifier
  URITemplateRouterDuplicateOperationIdError(
      const std::string_view operation_id)
      : operation_id_{operation_id} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "Duplicate operation identifier";
  }

  /// Get the conflicting operation identifier
  [[nodiscard]] auto operation_id() const noexcept -> const std::string & {
    return this->operation_id_;
  }

private:
  std::string operation_id_;
};

/// @ingroup uritemplate
/// An error that represents a failure to save the router to disk
class SOURCEMETA_CORE_URITEMPLATE_EXPORT URITemplateRouterSaveError
    : public std::exception {
public:
  /// Construct an error given the target path and a descriptive message
  URITemplateRouterSaveError(std::filesystem::path path, const char *message)
      : path_{std::move(path)}, message_{message} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

  /// Get the path that could not be saved
  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

private:
  std::filesystem::path path_;
  const char *message_;
};

/// @ingroup uritemplate
/// An error that represents a failure to read the router from disk
class SOURCEMETA_CORE_URITEMPLATE_EXPORT URITemplateRouterReadError
    : public std::exception {
public:
  /// Construct an error given the target path
  URITemplateRouterReadError(std::filesystem::path path)
      : path_{std::move(path)} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "Failed to open router file for reading";
  }

  /// Get the path that could not be read
  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

private:
  std::filesystem::path path_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::core

#endif
