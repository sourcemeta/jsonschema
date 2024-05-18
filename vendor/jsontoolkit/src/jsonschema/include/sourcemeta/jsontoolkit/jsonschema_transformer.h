#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_TRANSFORMER_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_TRANSFORMER_H_

#if defined(__EMSCRIPTEN__) || defined(__Unikraft__)
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
#else
#include "jsonschema_export.h"
#endif

#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonpointer.h>

#include <variant> // std::variant
#include <vector>  // std::vector

namespace sourcemeta::jsontoolkit {

/// @ingroup jsonschema
/// Represents a schema transformation operation that consists in deleting a
/// property from a schema
struct SchemaTransformerOperationErase {
  const Pointer pointer;
};

/// @ingroup jsonschema
/// Represents a schema transformation operation that consists in adding a new
/// property to a schema
struct SchemaTransformerOperationAssign {
  const Pointer pointer;
};

/// @ingroup jsonschema
/// Represents a schema transformation operation that consists in replacing a
/// part of a schema
struct SchemaTransformerOperationReplace {
  const Pointer pointer;
};

/// @ingroup jsonschema
/// Represents a schema transformation operation
using SchemaTransformerOperation =
    std::variant<SchemaTransformerOperationErase,
                 SchemaTransformerOperationAssign,
                 SchemaTransformerOperationReplace>;

/// @ingroup jsonschema
/// This is a proxy class to intercept transformations applied to a schema. We
/// use it to keep track of what changed to fix up schema references.
class SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT SchemaTransformer {
public:
  /// Construct a transformer given a schema
  SchemaTransformer(JSON &schema);

  /// Get the underlying schema
  auto schema() const -> const JSON &;
  /// Trace the operations applied to the schema
  auto traces() const -> const std::vector<SchemaTransformerOperation> &;

  /// Replace a subschema with another value
  auto replace(const Pointer &path, const JSON &value) -> void;
  /// Replace a subschema with another value
  auto replace(const Pointer &path, JSON &&value) -> void;
  /// Assign an object property
  auto assign(const Pointer &path, const JSON::String &key,
              const JSON &value) -> void;
  /// Assign an object property
  auto assign(const Pointer &path, const JSON::String &key,
              JSON &&value) -> void;
  /// Remove an object property
  auto erase(const Pointer &path, const JSON::String &key) -> void;
  /// Remove multiple object properties
  template <typename Iterator>
  auto erase_keys(const Pointer &path, Iterator first, Iterator last) -> void {
    for (auto iterator = first; iterator != last; ++iterator) {
      this->erase(path, *iterator);
    }
  }

  // For convenience

  /// Replace a schema with another value
  auto replace(const JSON &value) -> void;
  /// Replace a schema with another value
  auto replace(JSON &&value) -> void;
  /// Assign an object property
  auto assign(const JSON::String &key, const JSON &value) -> void;
  /// Assign an object property
  auto assign(const JSON::String &key, JSON &&value) -> void;
  /// Remove an object property
  auto erase(const JSON::String &key) -> void;
  /// Remove multiple object properties
  template <typename Iterator>
  auto erase_keys(Iterator first, Iterator last) -> void {
    this->erase_keys(empty_pointer, first, last);
  }

private:
  JSON &data;
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  std::vector<SchemaTransformerOperation> operations;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};
} // namespace sourcemeta::jsontoolkit

#endif
