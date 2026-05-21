#ifndef SOURCEMETA_BLAZE_FOUNDATION_WALKER_H_
#define SOURCEMETA_BLAZE_FOUNDATION_WALKER_H_

#ifndef SOURCEMETA_BLAZE_FOUNDATION_EXPORT
#include <sourcemeta/blaze/foundation_export.h>
#endif

#include <sourcemeta/core/json.h>

#include <sourcemeta/blaze/foundation_types.h>

#include <cstdint>     // std::uint64_t
#include <optional>    // std::optional
#include <string_view> // std::string_view
#include <vector>      // std::vector

namespace sourcemeta::blaze {

/// @ingroup foundation
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
/// #include <sourcemeta/blaze/foundation.h>
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
///          sourcemeta::blaze::SchemaIterator{
///          document, sourcemeta::blaze::schema_walker,
///          sourcemeta::blaze::schema_resolver}) {
///   sourcemeta::core::prettify(
///     sourcemeta::core::get(document, entry.pointer), std::cout);
///   std::cout << "\n";
/// }
/// ```
class SOURCEMETA_BLAZE_FOUNDATION_EXPORT SchemaIterator {
private:
  using internal = typename std::vector<SchemaIteratorEntry>;

public:
  using const_iterator = typename internal::const_iterator;
  SchemaIterator(const sourcemeta::core::JSON &input,
                 const SchemaWalker &walker, const SchemaResolver &resolver,
                 std::string_view default_dialect = "");
  [[nodiscard]] auto begin() const -> const_iterator;
  [[nodiscard]] auto end() const -> const_iterator;
  [[nodiscard]] auto cbegin() const -> const_iterator;
  [[nodiscard]] auto cend() const -> const_iterator;

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  internal subschemas{};
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

/// @ingroup foundation
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
/// #include <sourcemeta/blaze/foundation.h>
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
///          sourcemeta::blaze::SchemaIteratorFlat{
///          document, sourcemeta::blaze::schema_walker,
///          sourcemeta::blaze::schema_resolver}) {
///   sourcemeta::core::prettify(
///     sourcemeta::core::get(document, entry.pointer), std::cout);
///   std::cout << "\n";
/// }
/// ```
class SOURCEMETA_BLAZE_FOUNDATION_EXPORT SchemaIteratorFlat {
private:
  using internal = typename std::vector<SchemaIteratorEntry>;

public:
  using const_iterator = typename internal::const_iterator;
  SchemaIteratorFlat(const sourcemeta::core::JSON &input,
                     const SchemaWalker &walker, const SchemaResolver &resolver,
                     std::string_view default_dialect = "");
  [[nodiscard]] auto begin() const -> const_iterator;
  [[nodiscard]] auto end() const -> const_iterator;
  [[nodiscard]] auto cbegin() const -> const_iterator;
  [[nodiscard]] auto cend() const -> const_iterator;

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  internal subschemas{};
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

/// @ingroup foundation
///
/// Return an iterator over the top-level keywords of a given JSON Schema
/// definition in the order in which an implementation must evaluate them.
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/blaze/foundation.h>
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
///          sourcemeta::blaze::SchemaKeywordIterator{
///          document, sourcemeta::blaze::schema_walker,
///          sourcemeta::blaze::schema_resolver}) {
///   sourcemeta::core::stringify(entry.pointer, std::cout);
///   std::cout << "\n";
/// }
/// ```
class SOURCEMETA_BLAZE_FOUNDATION_EXPORT SchemaKeywordIterator {
private:
  using internal = typename std::vector<SchemaIteratorEntry>;

public:
  using const_iterator = typename internal::const_iterator;
  SchemaKeywordIterator(const sourcemeta::core::JSON &input,
                        const SchemaWalker &walker,
                        const SchemaResolver &resolver,
                        std::string_view default_dialect = "");
  [[nodiscard]] auto begin() const -> const_iterator;
  [[nodiscard]] auto end() const -> const_iterator;
  [[nodiscard]] auto cbegin() const -> const_iterator;
  [[nodiscard]] auto cend() const -> const_iterator;

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  internal entries{};
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::blaze

#endif
