#ifndef SOURCEMETA_ALTERSCHEMA_ENGINE_TRANSFORMER_H_
#define SOURCEMETA_ALTERSCHEMA_ENGINE_TRANSFORMER_H_

#include "engine_export.h"

#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonpointer.h>

#include <variant> // std::variant
#include <vector>  // std::vector

namespace sourcemeta::alterschema {

/// @ingroup engine
/// Represents a schema transformation operation that consists in deleting a
/// property from a schema
struct OperationErase {
  const sourcemeta::jsontoolkit::Pointer pointer;
};

/// @ingroup engine
/// Represents a schema transformation operation that consists in adding a new
/// property to a schema
struct OperationAssign {
  const sourcemeta::jsontoolkit::Pointer pointer;
};

/// @ingroup engine
/// Represents a schema transformation operation that consists in replacing a
/// part of a schema
struct OperationReplace {
  const sourcemeta::jsontoolkit::Pointer pointer;
};

/// @ingroup engine
/// Represents a schema transformation operation
using Operation =
    std::variant<OperationErase, OperationAssign, OperationReplace>;

/// @ingroup engine
/// This is a proxy class to intercept transformations applied to a schema. We
/// use it to keep track of what changed to fix up schema references.
class SOURCEMETA_ALTERSCHEMA_ENGINE_EXPORT Transformer {
public:
  /// Construct a transformer given a schema
  Transformer(sourcemeta::jsontoolkit::JSON &schema);

  /// Get the underlying schema
  auto schema() const -> const sourcemeta::jsontoolkit::JSON &;
  /// Trace the operations applied to the schema
  auto traces() const -> const std::vector<Operation> &;

  /// Replace a subschema with another value
  auto replace(const sourcemeta::jsontoolkit::Pointer &path,
               const sourcemeta::jsontoolkit::JSON &value) -> void;
  /// Replace a subschema with another value
  auto replace(const sourcemeta::jsontoolkit::Pointer &path,
               sourcemeta::jsontoolkit::JSON &&value) -> void;
  /// Assign an object property
  auto assign(const sourcemeta::jsontoolkit::Pointer &path,
              const sourcemeta::jsontoolkit::JSON::String &key,
              const sourcemeta::jsontoolkit::JSON &value) -> void;
  /// Assign an object property
  auto assign(const sourcemeta::jsontoolkit::Pointer &path,
              const sourcemeta::jsontoolkit::JSON::String &key,
              sourcemeta::jsontoolkit::JSON &&value) -> void;
  /// Remove an object property
  auto erase(const sourcemeta::jsontoolkit::Pointer &path,
             const sourcemeta::jsontoolkit::JSON::String &key) -> void;
  /// Remove multiple object properties
  template <typename Iterator>
  auto erase_keys(const sourcemeta::jsontoolkit::Pointer &path, Iterator first,
                  Iterator last) -> void {
    for (auto iterator = first; iterator != last; ++iterator) {
      this->erase(path, *iterator);
    }
  }

  // For convenience

  /// Replace a schema with another value
  auto replace(const sourcemeta::jsontoolkit::JSON &value) -> void;
  /// Replace a schema with another value
  auto replace(sourcemeta::jsontoolkit::JSON &&value) -> void;
  /// Assign an object property
  auto assign(const sourcemeta::jsontoolkit::JSON::String &key,
              const sourcemeta::jsontoolkit::JSON &value) -> void;
  /// Assign an object property
  auto assign(const sourcemeta::jsontoolkit::JSON::String &key,
              sourcemeta::jsontoolkit::JSON &&value) -> void;
  /// Remove an object property
  auto erase(const sourcemeta::jsontoolkit::JSON::String &key) -> void;
  /// Remove multiple object properties
  template <typename Iterator>
  auto erase_keys(Iterator first, Iterator last) -> void {
    this->erase_keys(sourcemeta::jsontoolkit::empty_pointer, first, last);
  }

private:
  sourcemeta::jsontoolkit::JSON &data;
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  std::vector<Operation> operations;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};
} // namespace sourcemeta::alterschema

#endif
