#ifndef SOURCEMETA_CORE_JSONPOINTER_PROXY_H_
#define SOURCEMETA_CORE_JSONPOINTER_PROXY_H_

#ifndef SOURCEMETA_CORE_JSONPOINTER_EXPORT
#include <sourcemeta/core/jsonpointer_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer_pointer.h>

#include <variant> // std::variant
#include <vector>  // std::vector

namespace sourcemeta::core {

/// @ingroup jsonpointer
/// This is a proxy class to intercept transformations applied to a document.
template <typename PointerT> class GenericPointerProxy {
public:
  GenericPointerProxy(JSON &document) : data{document} {}

  /// Represents a transformation operation that consists in deleting a property
  /// from a document
  struct OperationErase {
    const PointerT pointer;
  };

  /// Represents a transformation operation that consists in adding a new
  /// property to a document
  struct OperationAssign {
    const PointerT pointer;
  };

  /// Represents a transformation operation that consists in replacing a part of
  /// a document
  struct OperationReplace {
    const PointerT pointer;
  };

  /// Represents a transformation operation
  using Operation =
      std::variant<OperationErase, OperationAssign, OperationReplace>;

  /// Get the underlying document
  auto value() const -> const JSON & { return this->data; }

  /// Trace the operations applied to the document
  auto traces() const -> const std::vector<Operation> & {
    return this->operations;
  }

  /// Replace a subdocument with another value
  auto replace(const PointerT &path, const JSON &value) -> void {
    // TODO: Check that the path exists with an assert
    set(this->data, path, value);
    this->operations.push_back(OperationReplace{path});
  }

  /// Replace a subdocument with another value
  auto replace(const PointerT &path, JSON &&value) -> void {
    // TODO: Check that the path exists with an assert
    set(this->data, path, std::move(value));
    this->operations.push_back(OperationReplace{path});
  }

  /// Assign an object property
  auto assign(const PointerT &path, const JSON::String &key, const JSON &value)
      -> void {
    const auto destination{path.concat({key})};
    // TODO: Check that the path DOES NOT exist with an assert
    get(this->data, path).assign(key, value);
    this->operations.push_back(OperationAssign{path.concat({key})});
  }

  /// Assign an object property
  auto assign(const PointerT &path, const JSON::String &key, JSON &&value)
      -> void {
    // TODO: Check that the path DOES NOT exist with an assert
    get(this->data, path).assign(key, std::move(value));
    this->operations.push_back(OperationAssign{path.concat({key})});
  }

  /// Remove an object property
  auto erase(const PointerT &path, const JSON::String &key) -> void {
    // TODO: Check that the path exists with an assert
    get(this->data, path).erase(key);
    this->operations.push_back(OperationErase{path.concat({key})});
  }

  /// Remove multiple object properties
  template <typename Iterator>
  auto erase_keys(const PointerT &path, Iterator first, Iterator last) -> void {
    for (auto iterator = first; iterator != last; ++iterator) {
      this->erase(path, *iterator);
    }
  }

  // For convenience

  /// Replace a schema with another value
  auto replace(const JSON &value) -> void { this->replace(PointerT{}, value); }

  /// Replace a schema with another value
  auto replace(JSON &&value) -> void {
    this->replace(PointerT{}, std::move(value));
  }

  /// Assign an object property
  auto assign(const JSON::String &key, const JSON &value) -> void {
    this->assign(PointerT{}, key, value);
  }

  /// Assign an object property
  auto assign(const JSON::String &key, JSON &&value) -> void {
    this->assign(PointerT{}, key, std::move(value));
  }

  /// Remove an object property
  auto erase(const JSON::String &key) -> void { this->erase(PointerT{}, key); }

  /// Remove multiple object properties
  template <typename Iterator>
  auto erase_keys(Iterator first, Iterator last) -> void {
    this->erase_keys(PointerT{}, first, last);
  }

private:
  JSON &data;
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
} // namespace sourcemeta::core

#endif
