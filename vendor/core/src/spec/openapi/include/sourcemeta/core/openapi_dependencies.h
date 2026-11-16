#ifndef SOURCEMETA_CORE_OPENAPI_DEPENDENCIES_H_
#define SOURCEMETA_CORE_OPENAPI_DEPENDENCIES_H_

#ifndef SOURCEMETA_CORE_OPENAPI_EXPORT
#include <sourcemeta/core/openapi_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>

#include <sourcemeta/core/openapi_types.h>

#include <cstdint>     // std::uint64_t
#include <functional>  // std::function
#include <limits>      // std::numeric_limits
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup openapi
///
/// What one reference reaches, whether a description or a schema of the span
/// made it. Several of them may name one document, as what is reported is
/// where a reference sits rather than what analysis had to fetch. Whatever a
/// member carries points into something analysis owns only for the call that
/// reports it, so a callback keeping any of them past its own return copies
/// them out
struct OpenAPIDependency {
  /// The identifier the document making the reference goes by, which is
  /// nothing at all where it declares none and the caller named none
  JSON::StringView origin;
  /// Where in that document the reference sits
  const Pointer &pointer;
  /// Where it points, resolved and canonicalised, without its fragment
  JSON::StringView destination;
  /// The document that the reference leads to, which is there only where this
  /// is the reference that had analysis resolve it. Analysis lets go of what it
  /// read as soon as it is done with it, so a reference reaching a document the
  /// span already holds, and one that nothing answered for until another
  /// document settled it, carry nothing here
  const JSON *document;
  /// What the position spelling it expects to find at the far end
  OpenAPIObjectKind expected;
};

/// @ingroup openapi
/// A callback to report every dependency that analysis finds
using OpenAPIDependencyCallback =
    std::function<void(const OpenAPIDependency &)>;

/// @ingroup openapi
///
/// Everything dependency analysis takes beyond the description and how to read
/// it
struct OpenAPIDependencyOptions {
  /// The URI the document was retrieved from, which its relative references
  /// resolve against unless it declares a base of its own
  std::string_view default_base{};
  /// The most locations analysis may register before it throws
  /// sourcemeta::core::OpenAPIFrameLimitError
  std::uint64_t max_locations{std::numeric_limits<std::uint64_t>::max()};
};

/// @ingroup openapi
///
/// Report every reference an OpenAPI Description makes to another document,
/// transitively, whether this specification gives it meaning or JSON Schema
/// does. A document that several references name is reported once for each of
/// them, which leaves a caller wanting one edge per document to take it from
/// there. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/openapi.h>
///
/// // A resolver that knows the other document of this description
/// static auto shared_resolver(std::string_view identifier)
///     -> sourcemeta::core::OpenAPIResolverResult {
///   if (identifier == "https://www.example.com/shared.json") {
///     return sourcemeta::core::parse_json(R"JSON({
///       "openapi": "3.1.1",
///       "info": { "title": "Shared", "version": "1.0.0" },
///       "paths": { "/thing": { "description": "Shared" } }
///     })JSON");
///   }
///
///   return std::nullopt;
/// }
///
/// const auto document = sourcemeta::core::parse_json(R"JSON({
///   "openapi": "3.1.1",
///   "info": { "title": "Example", "version": "1.0.0" },
///   "paths": {
///     "/thing": {
///       "$ref": "https://www.example.com/shared.json#/paths/~1thing"
///     }
///   }
/// })JSON");
///
/// sourcemeta::core::openapi_dependencies(document,
///   sourcemeta::core::schema_walker, sourcemeta::core::schema_resolver,
///   shared_resolver,
///   [](const auto &dependency) {
///     // Do something with the information
///   });
/// ```
///
/// Only the reference that had analysis resolve a document carries it, which
/// leaves one reaching a document the span already holds reported with nothing
/// to fetch. A reference that nothing answers for when it is read leads
/// somewhere only once a document that arrived later settles it, so that one is
/// reported after the references of every document read in between, and not at
/// all where it ends analysis instead. A reference that names the document it
/// sits in is that document's own business rather than a dependency, and is
/// reported nowhere. An official JSON Schema dialect is reported where a
/// reference names one and left out where a dialect declaration does, and
/// neither is read.
///
/// Analysis throws where it cannot go on, which is wherever the span turns out
/// not to describe one API: a reference nothing answers for, a document that
/// cannot be read as what reached it, or more locations than the allowance
/// permits. Returns what it registered against that allowance, which falls
/// short of what a second run needs wherever a schema walk raised and another
/// document answered for it afterwards.
SOURCEMETA_CORE_OPENAPI_EXPORT
auto openapi_dependencies(const JSON &document, const SchemaWalker &walker,
                          const SchemaResolver &schema_resolver,
                          const OpenAPIResolver &resolver,
                          const OpenAPIDependencyCallback &callback,
                          const OpenAPIDependencyOptions &options = {})
    -> std::uint64_t;

} // namespace sourcemeta::core

#endif
