#ifndef SOURCEMETA_CORE_JSONSCHEMA_ERROR_H
#define SOURCEMETA_CORE_JSONSCHEMA_ERROR_H

#ifndef SOURCEMETA_CORE_JSONSCHEMA_EXPORT
#include <sourcemeta/core/jsonschema_export.h>
#endif

#include <sourcemeta/core/error.h>
#include <sourcemeta/core/jsonpointer.h>

#include <cstdint>     // std::uint64_t
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

SOURCEMETA_CORE_ERROR_WARNINGS_PUSH

/// @ingroup jsonschema
/// An error that represents a general schema error event
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaError : public Error {
public:
  /// Create a general schema error
  SchemaError(const StaticString message) noexcept : Error{message} {}

private:
};

/// @ingroup jsonschema
/// An error that represents a schema resolution failure event
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaResolutionError : public Error {
public:
  /// Create a schema resolution error
  SchemaResolutionError(const StaticString message,
                        const std::string_view identifier)
      : Error{message}, identifier_{identifier} {}

  /// The identifier that could not be resolved
  [[nodiscard]] auto identifier() const noexcept -> std::string_view {
    return this->identifier_;
  }

private:
  std::string identifier_;
};

/// @ingroup jsonschema
/// An error that represents a relative meta-schema resolution failure event
/// Relative references to meta-schemas are invalid as per the specification
/// See https://json-schema.org/draft/2020-12/json-schema-core#section-8.1.1-2
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaRelativeMetaschemaResolutionError
    : public SchemaResolutionError {
public:
  /// Create a relative meta-schema resolution error
  SchemaRelativeMetaschemaResolutionError(const std::string_view identifier)
      : SchemaResolutionError{"Relative meta-schema URIs are not valid "
                              "according to the JSON Schema specification",
                              identifier} {}
};

/// @ingroup jsonschema
/// An error that represents a schema vocabulary error
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaVocabularyError : public Error {
public:
  /// Create a vocabulary error
  SchemaVocabularyError(const StaticString message, const std::string_view uri)
      : Error{message}, uri_{uri} {}

  /// The vocabulary that caused the error
  [[nodiscard]] auto uri() const noexcept -> std::string_view {
    return this->uri_;
  }

private:
  std::string uri_;
};

/// @ingroup jsonschema
/// An error that represents a schema resolution failure event
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaReferenceError : public Error {
public:
  /// Create a reference error
  SchemaReferenceError(const StaticString message,
                       const std::string_view identifier,
                       sourcemeta::core::Pointer schema_location)
      : Error{message}, identifier_{identifier},
        schema_location_{std::move(schema_location)} {}

  /// The reference that caused the error
  [[nodiscard]] auto identifier() const noexcept -> std::string_view {
    return this->identifier_;
  }

  /// Where the problem is, as a pointer from the root of the document
  /// that the schema came from
  [[nodiscard]] auto location() const noexcept
      -> const sourcemeta::core::Pointer & {
    return this->schema_location_;
  }

private:
  std::string identifier_;
  sourcemeta::core::Pointer schema_location_;
};

/// @ingroup jsonschema
/// An error that represents that the dialect of the schema could not determined
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaUnknownDialectError
    : public Error {
public:
  SchemaUnknownDialectError() noexcept
      : Error{"Could not determine the dialect of the schema"} {}
};

/// @ingroup jsonschema
/// An error that represents that the base dialect of the schema could not
/// determined
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaUnknownBaseDialectError
    : public Error {
public:
  SchemaUnknownBaseDialectError() noexcept
      : Error{"Could not determine the base dialect of the schema"} {}
};

/// @ingroup jsonschema
/// In JSON Schema Draft 7 and older, a schema that defines `$ref` is a
/// reference object where every other keywords are ignored
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaReferenceObjectResourceError
    : public Error {
public:
  /// Create a reference object resource error
  SchemaReferenceObjectResourceError(const std::string_view identifier)
      : Error{"A schema with a top-level `$ref` in JSON Schema Draft 7 and "
              "older dialects ignores every sibling keywords (like "
              "identifiers and meta-schema declarations) and therefore many "
              "operations, like bundling, are not possible without undefined "
              "behavior"},
        identifier_{identifier} {}

  /// The reference that caused the error
  [[nodiscard]] auto identifier() const noexcept -> std::string_view {
    return this->identifier_;
  }

private:
  std::string identifier_;
};

/// @ingroup jsonschema
/// A schema names the dialect it is written against through a keyword, and the
/// dialect it names may be one that gives that very keyword no meaning. Such a
/// declaration asserts the conditions under which it could not have been
/// written, so no reading of the schema honours it. The caller has to name
/// such a dialect from the outside instead
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaDialectImpossibleError
    : public Error {
public:
  /// Create a dialect impossibility error
  SchemaDialectImpossibleError(const std::string_view keyword,
                               const std::string_view dialect,
                               sourcemeta::core::Pointer schema_location)
      : Error{"The dialect that this schema declares does not define the "
              "keyword that declares it"},
        keyword_{keyword}, dialect_{dialect},
        schema_location_{std::move(schema_location)} {}

  /// The keyword that names the dialect
  [[nodiscard]] auto keyword() const noexcept -> std::string_view {
    return this->keyword_;
  }

  /// The dialect that the schema names
  [[nodiscard]] auto dialect() const noexcept -> std::string_view {
    return this->dialect_;
  }

  /// Where the declaration is, as a pointer from the root of the document
  /// that the schema came from
  [[nodiscard]] auto location() const noexcept
      -> const sourcemeta::core::Pointer & {
    return this->schema_location_;
  }

private:
  std::string keyword_;
  std::string dialect_;
  sourcemeta::core::Pointer schema_location_;
};

/// @ingroup jsonschema
/// An error that represents a schema keyword error
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaKeywordError : public Error {
public:
  /// Create a keyword error
  SchemaKeywordError(const StaticString message, const std::string_view keyword,
                     const std::string_view value)
      : Error{message}, keyword_{keyword}, value_{value} {}

  /// The value that caused the error
  [[nodiscard]] auto value() const noexcept -> std::string_view {
    return this->value_;
  }

  /// The keyword that caused the error
  [[nodiscard]] auto keyword() const noexcept -> std::string_view {
    return this->keyword_;
  }

private:
  std::string keyword_;
  std::string value_;
};

/// @ingroup jsonschema
/// An error that represents a place bundling was told to put schemas into but
/// cannot
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaContainerError : public Error {
public:
  /// Create a container error
  SchemaContainerError(const StaticString message,
                       sourcemeta::core::Pointer location)
      : Error{message}, location_{std::move(location)} {}

  /// Where the container is, as a pointer from the root of the document that
  /// was being bundled
  [[nodiscard]] auto location() const noexcept
      -> const sourcemeta::core::Pointer & {
    return this->location_;
  }

private:
  sourcemeta::core::Pointer location_;
};

/// @ingroup jsonschema
/// An error that represents a schema frame error
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaFrameError : public Error {
public:
  /// Create a framing error
  SchemaFrameError(const StaticString message,
                   const std::string_view identifier)
      : Error{message}, identifier_{identifier} {}

  /// The identifier of the schema that caused the error
  [[nodiscard]] auto identifier() const noexcept -> std::string_view {
    return this->identifier_;
  }

private:
  std::string identifier_;
};

/// @ingroup jsonschema
/// An error that represents a schema that needs more frame locations than the
/// caller was willing to spend on analysing it
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaFrameLimitError : public Error {
public:
  /// Create a framing limit error
  SchemaFrameLimitError(const std::uint64_t limit) noexcept
      : Error{"The schema exceeds the maximum number of frame locations"},
        limit_{limit} {}

  /// The maximum number of locations that framing was allowed to register
  [[nodiscard]] auto limit() const noexcept -> std::uint64_t {
    return this->limit_;
  }

private:
  std::uint64_t limit_;
};

/// @ingroup jsonschema
/// An error that represents a schema anchor collision error
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaAnchorCollisionError
    : public Error {
public:
  /// Create an anchor collision error
  SchemaAnchorCollisionError(const std::string_view identifier,
                             sourcemeta::core::Pointer location,
                             sourcemeta::core::Pointer other)
      : Error{"Schema anchor already exists"}, identifier_{identifier},
        location_(std::move(location)), other_(std::move(other)) {}

  /// The anchor that collided
  [[nodiscard]] auto identifier() const noexcept -> std::string_view {
    return this->identifier_;
  }

  /// Where the problem is, as a pointer from the root of the document
  /// that the schema came from
  [[nodiscard]] auto location() const noexcept
      -> const sourcemeta::core::Pointer & {
    return this->location_;
  }

  /// The other place that declares the same anchor
  [[nodiscard]] auto other() const noexcept
      -> const sourcemeta::core::Pointer & {
    return this->other_;
  }

private:
  std::string identifier_;
  sourcemeta::core::Pointer location_;
  sourcemeta::core::Pointer other_;
};

SOURCEMETA_CORE_ERROR_WARNINGS_POP

} // namespace sourcemeta::core

#endif
