#ifndef SOURCEMETA_CORE_JSONSCHEMA_ERROR_H
#define SOURCEMETA_CORE_JSONSCHEMA_ERROR_H

#ifndef SOURCEMETA_CORE_JSONSCHEMA_EXPORT
#include <sourcemeta/core/jsonschema_export.h>
#endif

#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/uri.h>

#include <exception>   // std::exception
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup jsonschema
/// An error that represents a general schema error event
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaError : public std::exception {
public:
  SchemaError(const char *message) : message_{message} {}
  SchemaError(std::string message) = delete;
  SchemaError(std::string &&message) = delete;
  SchemaError(std::string_view message) = delete;

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

private:
  const char *message_;
};

/// @ingroup jsonschema
/// An error that represents a schema resolution failure event
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaResolutionError
    : public std::exception {
public:
  SchemaResolutionError(const std::string_view identifier, const char *message)
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

/// @ingroup jsonschema
/// An error that represents a relative meta-schema resolution failure event
/// Relative references to meta-schemas are invalid as per the specification
/// See https://json-schema.org/draft/2020-12/json-schema-core#section-8.1.1-2
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaRelativeMetaschemaResolutionError
    : public SchemaResolutionError {
public:
  SchemaRelativeMetaschemaResolutionError(const std::string_view identifier)
      : SchemaResolutionError{identifier,
                              "Relative meta-schema URIs are not valid "
                              "according to the JSON Schema specification"} {}
};

/// @ingroup jsonschema
/// An error that represents a schema vocabulary error
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaVocabularyError
    : public std::exception {
public:
  SchemaVocabularyError(const std::string_view uri, const char *message)
      : uri_{uri}, message_{message} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

  [[nodiscard]] auto uri() const noexcept -> std::string_view {
    return this->uri_;
  }

private:
  std::string uri_;
  const char *message_;
};

/// @ingroup jsonschema
/// An error that represents a schema resolution failure event
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaReferenceError
    : public std::exception {
public:
  SchemaReferenceError(const std::string_view identifier,
                       Pointer schema_location, const char *message)
      : identifier_{identifier}, schema_location_{std::move(schema_location)},
        message_{message} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

  [[nodiscard]] auto identifier() const noexcept -> std::string_view {
    return this->identifier_;
  }

  [[nodiscard]] auto location() const noexcept -> const Pointer & {
    return this->schema_location_;
  }

private:
  std::string identifier_;
  Pointer schema_location_;
  const char *message_;
};

/// @ingroup jsonschema
/// An error that represents a broken schema resolution event
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaBrokenReferenceError
    : public SchemaReferenceError {
  using SchemaReferenceError::SchemaReferenceError;
};

/// @ingroup jsonschema
/// An error that represents that a schema operation cannot continue
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaAbortError
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

/// @ingroup jsonschema
/// An error that represents that the dialect of the schema could not determined
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaUnknownDialectError
    : public std::exception {
public:
  SchemaUnknownDialectError() = default;

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "Could not determine the dialect of the schema";
  }
};

/// @ingroup jsonschema
/// An error that represents that the base dialect of the schema could not
/// determined
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaUnknownBaseDialectError
    : public std::exception {
public:
  SchemaUnknownBaseDialectError() = default;

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "Could not determine the base dialect of the schema";
  }
};

/// @ingroup jsonschema
/// An error that signifies that a transform rule was applied more than once
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaTransformRuleProcessedTwiceError
    : public std::exception {
public:
  SchemaTransformRuleProcessedTwiceError(const std::string_view name,
                                         Pointer location)
      : name_{name}, location_{std::move(location)} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "Transformation rules must only be processed once";
  }

  [[nodiscard]] auto name() const noexcept -> std::string_view {
    return this->name_;
  }

  [[nodiscard]] auto location() const noexcept -> const Pointer & {
    return this->location_;
  }

private:
  std::string name_;
  Pointer location_;
};

/// @ingroup jsonschema
/// In JSON Schema Draft 7 and older, a schema that defines `$ref` is a
/// reference object where every other keywords are ignored
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaReferenceObjectResourceError
    : public std::exception {
public:
  SchemaReferenceObjectResourceError(const std::string_view identifier)
      : identifier_{identifier} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "A schema with a top-level `$ref` in JSON Schema Draft 7 and older "
           "dialects ignores every sibling keywords (like identifiers and "
           "meta-schema declarations) and therefore many operations, like "
           "bundling, are not possible without undefined behavior";
  }

  [[nodiscard]] auto identifier() const noexcept -> std::string_view {
    return this->identifier_;
  }

private:
  std::string identifier_;
};

/// @ingroup jsonschema
/// An error that represents an unrecognized base dialect
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaBaseDialectError
    : public std::exception {
public:
  SchemaBaseDialectError(const std::string_view base_dialect)
      : base_dialect_{base_dialect} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "Unrecognized base dialect";
  }

  [[nodiscard]] auto base_dialect() const noexcept -> std::string_view {
    return this->base_dialect_;
  }

private:
  std::string base_dialect_;
};

/// @ingroup jsonschema
/// An error that represents a schema keyword error
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaKeywordError
    : public std::exception {
public:
  SchemaKeywordError(const std::string_view keyword,
                     const std::string_view value, const char *message)
      : keyword_{keyword}, value_{value}, message_{message} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

  [[nodiscard]] auto value() const noexcept -> std::string_view {
    return this->value_;
  }

  [[nodiscard]] auto keyword() const noexcept -> std::string_view {
    return this->keyword_;
  }

private:
  std::string keyword_;
  std::string value_;
  const char *message_;
};

/// @ingroup jsonschema
/// An error that represents a schema frame error
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaFrameError
    : public std::exception {
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

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::core

#endif
