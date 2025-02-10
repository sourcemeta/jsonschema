#ifndef SOURCEMETA_CORE_JSONSCHEMA_WALKER_H_
#define SOURCEMETA_CORE_JSONSCHEMA_WALKER_H_

#ifndef SOURCEMETA_CORE_JSONSCHEMA_EXPORT
#include <sourcemeta/core/jsonschema_export.h>
#endif

#include <sourcemeta/core/json.h>

#include <sourcemeta/core/jsonschema_types.h>

#include <cstdint>     // std::uint64_t
#include <map>         // std::map
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view
#include <vector>      // std::vector

namespace sourcemeta::core {

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
///   sourcemeta::core::parse_json(R"JSON({
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
///          document, sourcemeta::core::schema_official_walker,
///          sourcemeta::core::schema_official_resolver}) {
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
///   sourcemeta::core::parse_json(R"JSON({
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
///          document, sourcemeta::core::schema_official_walker,
///          sourcemeta::core::schema_official_resolver}) {
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
///   sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "object",
///   "properties": {},
///   "additionalProperties": true,
///   "patternProperties": {}
/// })JSON");
///
/// for (const auto &entry :
///          sourcemeta::core::SchemaKeywordIterator{
///          document, sourcemeta::core::schema_official_walker,
///          sourcemeta::core::schema_official_resolver}) {
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
