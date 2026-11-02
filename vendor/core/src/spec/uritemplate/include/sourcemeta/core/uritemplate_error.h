#ifndef SOURCEMETA_CORE_URITEMPLATE_ERROR_H_
#define SOURCEMETA_CORE_URITEMPLATE_ERROR_H_

#ifndef SOURCEMETA_CORE_URITEMPLATE_EXPORT
#include <sourcemeta/core/uritemplate_export.h>
#endif

#include <sourcemeta/core/error.h>

#include <cstdint>     // std::uint64_t
#include <filesystem>  // std::filesystem::path
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace sourcemeta::core {

SOURCEMETA_CORE_ERROR_WARNINGS_PUSH

/// @ingroup uritemplate
/// An error that represents a URI Template parsing failure
class SOURCEMETA_CORE_URITEMPLATE_EXPORT URITemplateParseError : public Error {
public:
  /// Construct an error given the column number where parsing failed
  URITemplateParseError(const std::uint64_t column) noexcept
      : Error{"The input is not a valid URI Template"}, column_{column} {}

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
    : public Error {
public:
  /// Construct an error with a descriptive message
  URITemplateExpansionError(const StaticString message) noexcept
      : Error{message} {}
};

/// @ingroup uritemplate
/// An error that represents a variable name mismatch when adding routes
class SOURCEMETA_CORE_URITEMPLATE_EXPORT URITemplateRouterVariableMismatchError
    : public Error {
public:
  /// Construct an error given the existing and conflicting variable names
  URITemplateRouterVariableMismatchError(const std::string_view left,
                                         const std::string_view right)
      : Error{"Variable name mismatch when adding route"}, left_{left},
        right_{right} {}

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
    : public Error {
public:
  /// Construct an error given a descriptive message and the offending segment
  URITemplateRouterInvalidSegmentError(const StaticString message,
                                       const std::string_view segment)
      : Error{message}, segment_{segment} {}

  /// Get the offending segment
  [[nodiscard]] auto segment() const noexcept -> const std::string & {
    return this->segment_;
  }

private:
  std::string segment_;
};

/// @ingroup uritemplate
/// An error that represents an operation identifier that does not match the
/// permitted format
class SOURCEMETA_CORE_URITEMPLATE_EXPORT
    URITemplateRouterInvalidOperationIdError : public Error {
public:
  /// Construct an error given the offending operation identifier
  URITemplateRouterInvalidOperationIdError(const std::string_view operation_id)
      : Error{"Invalid operation identifier"}, operation_id_{operation_id} {}

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
    URITemplateRouterDuplicateOperationIdError : public Error {
public:
  /// Construct an error given the conflicting operation identifier
  URITemplateRouterDuplicateOperationIdError(
      const std::string_view operation_id)
      : Error{"Duplicate operation identifier"}, operation_id_{operation_id} {}

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
    : public Error {
public:
  /// Construct an error given a descriptive message and the target path
  URITemplateRouterSaveError(const StaticString message,
                             std::filesystem::path path)
      : Error{message}, path_{std::move(path)} {}

  /// Get the path that could not be saved
  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

private:
  std::filesystem::path path_;
};

/// @ingroup uritemplate
/// An error that represents a failure to read the router from disk
class SOURCEMETA_CORE_URITEMPLATE_EXPORT URITemplateRouterReadError
    : public Error {
public:
  /// Construct an error given the target path
  URITemplateRouterReadError(std::filesystem::path path)
      : Error{"Failed to open router file for reading"},
        path_{std::move(path)} {}

  /// Get the path that could not be read
  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

private:
  std::filesystem::path path_;
};

SOURCEMETA_CORE_ERROR_WARNINGS_POP

} // namespace sourcemeta::core

#endif
