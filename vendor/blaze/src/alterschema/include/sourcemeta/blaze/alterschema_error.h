#ifndef SOURCEMETA_BLAZE_ALTERSCHEMA_ERROR_H_
#define SOURCEMETA_BLAZE_ALTERSCHEMA_ERROR_H_

#ifndef SOURCEMETA_BLAZE_ALTERSCHEMA_EXPORT
#include <sourcemeta/blaze/alterschema_export.h>
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

/// @ingroup alterschema
/// An error that represents a missing schema rule name
class SOURCEMETA_BLAZE_ALTERSCHEMA_EXPORT SchemaRuleMissingNameError
    : public std::exception {
public:
  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "The schema rule is missing a title";
  }
};

/// @ingroup alterschema
/// An error that represents an invalid schema rule name. The name must
/// consist only of lowercase ASCII letters, digits, underscores, or slashes.
class SOURCEMETA_BLAZE_ALTERSCHEMA_EXPORT SchemaRuleInvalidNameError
    : public std::exception {
public:
  SchemaRuleInvalidNameError(const std::string_view identifier,
                             const char *message)
      : identifier_{identifier}, message_{message} {}
  SchemaRuleInvalidNameError(const std::string_view identifier,
                             std::string message) = delete;
  SchemaRuleInvalidNameError(const std::string_view identifier,
                             std::string &&message) = delete;
  SchemaRuleInvalidNameError(const std::string_view identifier,
                             std::string_view message) = delete;

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

  [[nodiscard]] auto identifier() const noexcept -> const std::string & {
    return this->identifier_;
  }

private:
  std::string identifier_;
  const char *message_;
};

/// @ingroup alterschema
/// An error that represents a schema rule name that does not match
/// the required pattern
class SOURCEMETA_BLAZE_ALTERSCHEMA_EXPORT SchemaRuleInvalidNamePatternError
    : public std::exception {
public:
  SchemaRuleInvalidNamePatternError(const std::string_view identifier,
                                    const std::string_view regex)
      : identifier_{identifier}, regex_{regex} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "The schema rule name does not match the required pattern";
  }

  [[nodiscard]] auto identifier() const noexcept -> const std::string & {
    return this->identifier_;
  }

  [[nodiscard]] auto regex() const noexcept -> const std::string & {
    return this->regex_;
  }

private:
  std::string identifier_;
  std::string regex_;
};

/// @ingroup alterschema
/// An error that represents that a schema operation cannot continue
class SOURCEMETA_BLAZE_ALTERSCHEMA_EXPORT SchemaAbortError
    : public std::exception {
public:
  SchemaAbortError(const char *message) : message_{message} {}
  SchemaAbortError(std::string message) = delete;
  SchemaAbortError(std::string &&message) = delete;
  SchemaAbortError(std::string_view message) = delete;

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

private:
  const char *message_;
};

/// @ingroup alterschema
/// An error that represents a broken schema reference after transformation
class SOURCEMETA_BLAZE_ALTERSCHEMA_EXPORT SchemaBrokenReferenceError
    : public std::exception {
public:
  SchemaBrokenReferenceError(const std::string_view identifier,
                             sourcemeta::core::Pointer schema_location,
                             const char *message)
      : identifier_{identifier}, schema_location_{std::move(schema_location)},
        message_{message} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

  [[nodiscard]] auto identifier() const noexcept -> std::string_view {
    return this->identifier_;
  }

  [[nodiscard]] auto location() const noexcept
      -> const sourcemeta::core::Pointer & {
    return this->schema_location_;
  }

private:
  std::string identifier_;
  sourcemeta::core::Pointer schema_location_;
  const char *message_;
};

/// @ingroup alterschema
/// An error that signifies that a transform rule was applied more than once
class SOURCEMETA_BLAZE_ALTERSCHEMA_EXPORT SchemaTransformRuleProcessedTwiceError
    : public std::exception {
public:
  SchemaTransformRuleProcessedTwiceError(const std::string_view name,
                                         sourcemeta::core::Pointer location)
      : name_{name}, location_{std::move(location)} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "Transformation rules must only be processed once";
  }

  [[nodiscard]] auto name() const noexcept -> std::string_view {
    return this->name_;
  }

  [[nodiscard]] auto location() const noexcept
      -> const sourcemeta::core::Pointer & {
    return this->location_;
  }

private:
  std::string name_;
  sourcemeta::core::Pointer location_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::blaze

#endif
