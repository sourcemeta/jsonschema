#ifndef SOURCEMETA_CORE_JSONSCHEMA_WALKER_H_
#define SOURCEMETA_CORE_JSONSCHEMA_WALKER_H_

#ifndef SOURCEMETA_CORE_JSONSCHEMA_EXPORT
#include <sourcemeta/core/jsonschema_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema_keywords.h>
#include <sourcemeta/core/jsonschema_resolver.h>

#include <cstdint>     // std::uint64_t
#include <functional>  // std::function
#include <map>         // std::map
#include <optional>    // std::optional
#include <set>         // std::set
#include <string>      // std::string
#include <string_view> // std::string_view
#include <vector>      // std::vector

namespace sourcemeta::core {

/// @ingroup jsonschema
/// A structure that encapsulates the result of walker over a specific keyword
struct SchemaWalkerResult {
  /// The walker strategy to continue traversing across the schema
  const KeywordType type;
  /// The vocabulary associated with the keyword, if any
  const std::optional<std::string> vocabulary;
  /// The keywords a given keyword depends on (if any) during the evaluation
  /// process
  const std::set<std::string> dependencies;
};

/// @ingroup jsonschema
///
/// For walking purposes, some functions need to understand which JSON Schema
/// keywords declare other JSON Schema definitions. To accomplish this in a
/// generic and flexible way that does not assume the use any vocabulary other
/// than `core`, these functions take a walker function as argument, of the type
/// sourcemeta::core::SchemaWalker.
///
/// For convenience, we provide the following default walkers:
///
/// - sourcemeta::core::default_schema_walker
/// - sourcemeta::core::schema_walker_none
using SchemaWalker = std::function<SchemaWalkerResult(
    std::string_view, const std::map<std::string, bool> &)>;

/// @ingroup jsonschema
/// A stub walker that doesn't walk
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
inline auto schema_walker_none(std::string_view,
                               const std::map<std::string, bool> &)
    -> sourcemeta::core::SchemaWalkerResult {
  return {KeywordType::Unknown, std::nullopt, {}};
}

/// @ingroup jsonschema
/// A default schema walker with support for a wide range of drafs
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto default_schema_walker(std::string_view keyword,
                           const std::map<std::string, bool> &vocabularies)
    -> sourcemeta::core::SchemaWalkerResult;

/// @ingroup jsonschema
/// An entry of a schema iterator.
struct SchemaIteratorEntry {
  Pointer pointer;
  std::optional<std::string> dialect;
  std::map<std::string, bool> vocabularies;
  std::optional<std::string> base_dialect;
  JSON value;
};

/// @ingroup jsonschema
///
/// Return an iterator over the subschemas of a given JSON Schema definition
/// according to the applicators understood by the provided walker function.
/// This walker recursively traverses over every subschema of
/// the JSON Schema definition, including the top-level schema, reporting back
/// each subschema.
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
/// #include <iostream>
///
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "object",
///   "properties": {
///     "foo": {
///       "type": "array",
///       "items": {
///         "type": "string"
///       }
///     }
///   }
/// })JSON");
///
/// for (const auto &entry :
///          sourcemeta::core::SchemaIterator{
///          document, sourcemeta::core::default_schema_walker,
///          sourcemeta::core::official_resolver}) {
///   sourcemeta::core::prettify(
///     sourcemeta::core::get(document, entry.pointer), std::cout);
///   std::cout << "\n";
/// }
/// ```
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaIterator {
private:
  using internal = typename std::vector<SchemaIteratorEntry>;

public:
  using const_iterator = typename internal::const_iterator;
  SchemaIterator(
      const JSON &input, const SchemaWalker &walker,
      const SchemaResolver &resolver,
      const std::optional<std::string> &default_dialect = std::nullopt);
  auto begin() const -> const_iterator;
  auto end() const -> const_iterator;
  auto cbegin() const -> const_iterator;
  auto cend() const -> const_iterator;

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  internal subschemas;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

/// @ingroup jsonschema
///
/// Return an iterator over the subschemas of a given JSON Schema definition
/// according to the applicators understood by the provided walker function.
/// This walker traverse over the first-level of subschemas of the JSON Schema
/// definition, ignoring the top-level schema and reporting back each subschema.
///
/// Note that we don't promise any specific walking ordering.
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
/// #include <iostream>
///
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "object",
///   "properties": {
///     "foo": {
///       "type": "array",
///       "items": {
///         "type": "string"
///       }
///     }
///   }
/// })JSON");
///
/// for (const auto &entry :
///          sourcemeta::core::SchemaIteratorFlat{
///          document, sourcemeta::core::default_schema_walker,
///          sourcemeta::core::official_resolver}) {
///   sourcemeta::core::prettify(
///     sourcemeta::core::get(document, entry.pointer), std::cout);
///   std::cout << "\n";
/// }
/// ```
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaIteratorFlat {
private:
  using internal = typename std::vector<SchemaIteratorEntry>;

public:
  using const_iterator = typename internal::const_iterator;
  SchemaIteratorFlat(
      const JSON &input, const SchemaWalker &walker,
      const SchemaResolver &resolver,
      const std::optional<std::string> &default_dialect = std::nullopt);
  auto begin() const -> const_iterator;
  auto end() const -> const_iterator;
  auto cbegin() const -> const_iterator;
  auto cend() const -> const_iterator;

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  internal subschemas;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

/// @ingroup jsonschema
///
/// Calculate the priority of a keyword that determines the ordering in which a
/// JSON Schema implementation should evaluate keyword on a subschema. It does
/// so based on the keyword dependencies expressed in the schema walker. The
/// higher the priority, the more the evaluation of such keyword must be
/// delayed.
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
/// #include <cassert>
///
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "prefixItems": [ true, true ],
///   "items": false
/// })JSON");
///
/// const auto vocabularies{
///   sourcemeta::core::vocabularies(
///     document, sourcemeta::core::official_resolver)};
///
/// assert(sourcemeta::core::keyword_priority(
///   "prefixItems", vocabularies,
///   sourcemeta::core::default_schema_walker) == 0);
///
/// // The "items" keyword must be evaluated after the "prefixItems" keyword
/// assert(sourcemeta::core::keyword_priority(
///   "items", vocabularies,
///   sourcemeta::core::default_schema_walker) == 1);
/// ```
auto SOURCEMETA_CORE_JSONSCHEMA_EXPORT keyword_priority(
    std::string_view keyword, const std::map<std::string, bool> &vocabularies,
    const SchemaWalker &walker) -> std::uint64_t;

/// @ingroup jsonschema
///
/// Return an iterator over the top-level keywords of a given JSON Schema
/// definition in the order in which an implementation must evaluate them.
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
/// #include <iostream>
///
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "object",
///   "properties": {},
///   "additionalProperties": true,
///   "patternProperties": {}
/// })JSON");
///
/// for (const auto &entry :
///          sourcemeta::core::SchemaKeywordIterator{
///          document, sourcemeta::core::default_schema_walker,
///          sourcemeta::core::official_resolver}) {
///   sourcemeta::core::stringify(entry.pointer, std::cout);
///   std::cout << "\n";
/// }
/// ```
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaKeywordIterator {
private:
  using internal = typename std::vector<SchemaIteratorEntry>;

public:
  using const_iterator = typename internal::const_iterator;
  SchemaKeywordIterator(
      const JSON &input, const SchemaWalker &walker,
      const SchemaResolver &resolver,
      const std::optional<std::string> &default_dialect = std::nullopt);
  auto begin() const -> const_iterator;
  auto end() const -> const_iterator;
  auto cbegin() const -> const_iterator;
  auto cend() const -> const_iterator;

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  internal entries;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::core

#endif
