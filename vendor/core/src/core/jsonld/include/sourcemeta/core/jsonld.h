#ifndef SOURCEMETA_CORE_JSONLD_H_
#define SOURCEMETA_CORE_JSONLD_H_

#ifndef SOURCEMETA_CORE_JSONLD_EXPORT
#include <sourcemeta/core/jsonld_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonld_error.h>
#include <sourcemeta/core/jsonld_materialize.h>

#include <cstdint>    // std::uint8_t
#include <functional> // std::function
#include <optional>   // std::optional

/// @defgroup jsonld JSON-LD
/// @brief A JSON-LD 1.1 processor, with support for the JSON-LD 1.0 processing
/// mode.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/jsonld.h>
/// ```

namespace sourcemeta::core {

/// @ingroup jsonld
/// A resolver callback for loading remote JSON-LD contexts referenced during
/// expansion. Given an absolute IRI, it returns the referenced document, or no
/// value if it cannot be resolved.
using JSONLDResolver = std::function<std::optional<JSON>(JSON::StringView)>;

/// @ingroup jsonld
/// The JSON-LD processing mode
enum class JSONLDVersion : std::uint8_t {
  /// The JSON-LD 1.0 processing mode
  V1_0,
  /// The JSON-LD 1.1 processing mode
  V1_1
};

/// @ingroup jsonld
/// The reason expansion discarded a position in the input document
enum class JSONLDExpandDropReason : std::uint8_t {
  /// The active context does not define the key, and the key is not itself an
  /// absolute IRI, so it cannot name a property
  UndefinedKey,
  /// The key has the form of a keyword without being one, so no context may
  /// give it a meaning
  KeywordFormKey,
  /// The active context defines the key as a term that keeps no identifier,
  /// which retires it from expansion while leaving it to be redefined
  RetiredKey,
  /// A type that expands to no absolute IRI, which cannot name a type
  UndefinedType,
  /// A keyword that carries no expanded value, so expansion has nothing to add
  /// to the result for it
  UnhandledKeyword,
  /// A value object whose value is null
  NullValueObject,
  /// A value object that carries a language or a direction but no value
  IncompleteValueObject,
  /// A value with no property standing over it, where only a node object may
  /// stand
  FreeFloatingValue,
  /// A null written where a value was expected
  NullValue
};

/// @ingroup jsonld
/// A callback for reporting the input that expansion discards. Given why the
/// position was discarded, a pointer to it in the input document, and the
/// value that goes no further, neither of the latter two outliving the call.
///
/// A key the active context does not define is never refused. It is dropped
/// along with its value and everything nested below it (JSON-LD 1.1 API
/// Section 5.1.2 step 13.3), which leaves a mistyped key indistinguishable
/// from a document that says nothing, and is what this reports. See "A
/// JSON-LD Handbook" by Wolodymyr Pawlyshyn, chapter 2,
/// https://dr-json-ld.com/book/#expansion
using JSONLDExpandDropCallback = std::function<void(
    JSONLDExpandDropReason, const WeakPointer &, const JSON &)>;

/// @ingroup jsonld
///
/// Expand a JSON-LD document into its expanded form, resolving relative
/// references against the given base IRI and loading any remote context through
/// the given resolver. The result is always a JSON array. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonld.h>
/// #include <iostream>
///
/// const auto document{sourcemeta::core::parse_json(R"({
///   "@context": { "name": "https://schema.org/name" },
///   "name": "Sourcemeta"
/// })")};
///
/// const auto expanded{sourcemeta::core::jsonld_expand(document)};
/// sourcemeta::core::prettify(expanded, std::cout);
/// std::cout << std::endl;
/// ```
SOURCEMETA_CORE_JSONLD_EXPORT
auto jsonld_expand(const JSON &input, const JSON::StringView base_iri = "",
                   const JSONLDResolver &resolver = {},
                   const JSONLDVersion version = JSONLDVersion::V1_1,
                   const JSONLDExpandDropCallback &drop_callback = {}) -> JSON;

/// @ingroup jsonld
///
/// Expand a JSON-LD document, applying the given expansion context before the
/// document's own context, as if it had been prepended to the input. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonld.h>
/// #include <iostream>
///
/// const auto document{sourcemeta::core::parse_json(
///     R"({ "name": "Sourcemeta" })")};
/// const auto context{sourcemeta::core::parse_json(
///     R"({ "name": "https://schema.org/name" })")};
///
/// const auto expanded{sourcemeta::core::jsonld_expand(document, context)};
/// sourcemeta::core::prettify(expanded, std::cout);
/// std::cout << std::endl;
/// ```
SOURCEMETA_CORE_JSONLD_EXPORT
auto jsonld_expand(const JSON &input, const JSON &expand_context,
                   const JSON::StringView base_iri = "",
                   const JSONLDResolver &resolver = {},
                   const JSONLDVersion version = JSONLDVersion::V1_1,
                   const JSONLDExpandDropCallback &drop_callback = {}) -> JSON;

/// @ingroup jsonld
///
/// Determine whether a document is in the JSON-LD 1.1 expanded document form:
/// an array of node objects that carries no context and whose keys are all
/// absolute IRIs, blank node identifiers, or keywords. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonld.h>
/// #include <cassert>
///
/// const auto document{sourcemeta::core::parse_json(R"([
///   { "http://schema.org/name": [ { "@value": "Sourcemeta" } ] }
/// ])")};
///
/// assert(sourcemeta::core::jsonld_is_expanded(document));
/// ```
SOURCEMETA_CORE_JSONLD_EXPORT
auto jsonld_is_expanded(const JSON &document) -> bool;

/// @ingroup jsonld
///
/// Compact a JSON-LD document in expanded form against the given context to
/// produce a friendlier shape. The input must already be expanded, and the
/// result carries the given context. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonld.h>
/// #include <iostream>
///
/// const auto document{sourcemeta::core::parse_json(R"([
///   { "http://schema.org/name": [ { "@value": "Sourcemeta" } ] }
/// ])")};
///
/// const auto context{sourcemeta::core::parse_json(R"({
///   "name": "http://schema.org/name"
/// })")};
///
/// const auto compacted{sourcemeta::core::jsonld_compact(document, context)};
/// sourcemeta::core::prettify(compacted, std::cout);
/// std::cout << std::endl;
/// ```
SOURCEMETA_CORE_JSONLD_EXPORT
auto jsonld_compact(const JSON &input, const JSON &context,
                    const JSON::StringView base_iri = "",
                    const JSONLDResolver &resolver = {},
                    const JSONLDVersion version = JSONLDVersion::V1_1,
                    const bool compact_arrays = true,
                    const bool compact_to_relative = true) -> JSON;

/// @ingroup jsonld
///
/// Flatten a JSON-LD document in expanded form, collecting the properties of
/// each node into a single node object and labelling blank nodes. The input
/// must already be expanded. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonld.h>
/// #include <iostream>
///
/// const auto document{sourcemeta::core::parse_json(R"([
///   {
///     "@id": "http://example.com/a",
///     "http://example.com/b": [ { "@id": "http://example.com/c" } ]
///   }
/// ])")};
///
/// const auto flattened{sourcemeta::core::jsonld_flatten(document)};
/// sourcemeta::core::prettify(flattened, std::cout);
/// std::cout << std::endl;
/// ```
SOURCEMETA_CORE_JSONLD_EXPORT
auto jsonld_flatten(const JSON &input) -> JSON;

/// @ingroup jsonld
///
/// Flatten a JSON-LD document in expanded form and compact the result against
/// the given context. The input must already be expanded, and the result
/// carries the given context. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonld.h>
/// #include <iostream>
///
/// const auto document{sourcemeta::core::parse_json(R"([
///   { "http://schema.org/name": [ { "@value": "Sourcemeta" } ] }
/// ])")};
///
/// const auto context{sourcemeta::core::parse_json(R"({
///   "name": "http://schema.org/name"
/// })")};
///
/// const auto flattened{sourcemeta::core::jsonld_flatten(document, context)};
/// sourcemeta::core::prettify(flattened, std::cout);
/// std::cout << std::endl;
/// ```
SOURCEMETA_CORE_JSONLD_EXPORT
auto jsonld_flatten(const JSON &input, const JSON &context,
                    const JSON::StringView base_iri = "",
                    const JSONLDResolver &resolver = {},
                    const JSONLDVersion version = JSONLDVersion::V1_1,
                    const bool compact_arrays = true) -> JSON;

} // namespace sourcemeta::core

#endif
