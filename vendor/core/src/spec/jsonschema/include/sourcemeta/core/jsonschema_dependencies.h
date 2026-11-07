#ifndef SOURCEMETA_CORE_JSONSCHEMA_DEPENDENCIES_H_
#define SOURCEMETA_CORE_JSONSCHEMA_DEPENDENCIES_H_

#ifndef SOURCEMETA_CORE_JSONSCHEMA_EXPORT
#include <sourcemeta/core/jsonschema_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

#include <sourcemeta/core/jsonschema_frame.h>
#include <sourcemeta/core/jsonschema_types.h>

#include <cstdint>     // std::uint64_t
#include <functional>  // std::equal_to, std::function, std::hash
#include <limits>      // std::numeric_limits
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup jsonschema
///
/// A reference that a schema makes to a schema of another document. Every
/// member of this points into something that analysis owns for no longer than
/// the call that reports it, so a callback that keeps any of them past its own
/// return has to copy them out
struct SchemaDependency {
  /// The identifier that the document making the reference goes by, which for
  /// a document the walk read is the URI it was asked for, and for the one
  /// analysis started from is the identifier it declares, or the name the
  /// caller gave it, or nothing at all
  std::string_view origin;
  /// Where in that document the reference sits
  const sourcemeta::core::WeakPointer &pointer;
  /// The identifier of the document that the reference leads to
  std::string_view destination;
  /// The schema that the identifier was resolved to
  const sourcemeta::core::JSON &document;
};

/// @ingroup jsonschema
/// A callback to report every dependency that analysis finds
using SchemaDependencyCallback = std::function<void(const SchemaDependency &)>;

/// @ingroup jsonschema
///
/// Everything dependency analysis takes beyond the schema and how to read it.
/// As with sourcemeta::core::SchemaFrame, the dialect and the paths are read
/// for as long as analysis runs, so the caller owns them across the call,
/// while the identifier and the base are copied
struct SchemaDependenciesOptions {
  /// A predicate to ask whether the caller already accounts for a URI
  using AccountedPredicate = std::function<bool(std::string_view)>;
  /// A callback to report every URI that analysis comes to account for
  using AccountedCallback = std::function<void(std::string_view)>;

  /// The dialect to assume for a schema that declares none
  std::string_view default_dialect{};
  /// The identifier to assume for a schema that declares none, which names
  /// the whole document and so has no meaning where the paths below name
  /// anything other than the top of it. A wrapper takes the base below
  /// instead
  std::string_view default_id{};
  /// The paths to analyse within a schema wrapper. Every one of them must
  /// name a schema of the document, and passing paths that repeat or that sit
  /// inside one another is undefined behaviour. Naming a path puts everything
  /// outside it out of scope, so an identifier or a dialect that a schema
  /// enclosing it declares counts for nothing, and each path is read as a
  /// schema in its own right against the base and the dialect given here
  SchemaFrame::Paths paths{sourcemeta::core::EMPTY_WEAK_POINTER};
  /// The base URI that the document was retrieved from, which a relative
  /// reference within any of the given paths resolves against. As with
  /// sourcemeta::core::SchemaFrame, this does not claim that the document
  /// declares an identifier
  std::string_view default_base{};
  /// The maximum number of frame locations that analysis may register. How
  /// many schemas analysis ends up reading follows from what the resolver
  /// hands back rather than from the schema the caller passed in, and every
  /// frame that analysis constructs spends from this one limit, throwing
  /// sourcemeta::core::SchemaFrameLimitError once it runs out. Note that a
  /// document is read in full before anything charges for it, and that what
  /// a document costs to walk follows its size rather than how many locations
  /// it registers, so this bounds how many oversized schemas get read rather
  /// than whether one does. Bounding the size of a document remains the
  /// caller's to do
  std::uint64_t max_locations{std::numeric_limits<std::uint64_t>::max()};
  /// Whether the caller already accounts for a URI, which analysis asks
  /// before reading the document a reference names and again before deciding
  /// that a reference leads nowhere. Answering yes for one keeps the document
  /// it names from being either reported or read, so a caller answers for
  /// what it holds rather than for what it would rather not hear about. A
  /// dialect that a document declares is asked of the resolver by the framing
  /// that reading takes, which happens before any of this, so answering yes
  /// for one does not keep it from being resolved
  AccountedPredicate accounted{};
  /// Reports every URI that analysis comes to account for, whether it is one
  /// a dependency was reported for or one a document it read answers to of
  /// its own. A caller that walks several documents of one span keeps what
  /// this hands back and answers the predicate above from it, so that a
  /// document any of them reaches is read once and a reference that a
  /// document any of them read answers to settles for all of them
  AccountedCallback accounted_callback{};
};

/// @ingroup jsonschema
///
/// This function reports every other document that a JSON Schema reaches,
/// transitively, each of them once. A document that another one already
/// carries within itself is not one of them. Official schemas are reported
/// when a reference names them and left out when a dialect declaration does,
/// and neither is traversed into. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
///
/// // A custom resolver that knows about an additional schema
/// static auto test_resolver(std::string_view identifier)
///     -> sourcemeta::core::SchemaResolverResult {
///   if (identifier == "https://www.example.com/test") {
///     return sourcemeta::core::parse_json(R"JSON({
///       "$id": "https://www.example.com/test",
///       "$schema": "https://json-schema.org/draft/2020-12/schema",
///       "type": "string"
///     })JSON");
///   } else {
///     return sourcemeta::core::schema_resolver(identifier);
///   }
/// }
///
/// const sourcemeta::core::JSON document =
///     sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "items": { "$ref": "https://www.example.com/test" }
/// })JSON");
///
/// sourcemeta::core::schema_dependencies(document,
///   sourcemeta::core::schema_walker, test_resolver,
///   [](const auto &dependency) {
///     // Do something with the information
///   });
/// ```
///
/// Returns how many frame locations the analysis registered, which a caller
/// that spends from the same allowance elsewhere takes off its own count.
///
/// Analysis throws where it cannot go on. A dialect that nothing can produce
/// ends it, as does a document that cannot be read as a schema at all, and so
/// does a reference that nothing answers for, though only once every document
/// reached has been read, since one of them may still carry what an earlier
/// reference asked for. A reference that lands nowhere within a document the
/// walk went on to read is reported all the same, while one that lands nowhere
/// within the document making it ends analysis. Where the paths above confine
/// analysis to part of a document, what the rest of that document holds is
/// none of its business, so a reference into the part left out is neither
/// followed nor complained about.
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto schema_dependencies(const sourcemeta::core::JSON &schema,
                         const SchemaWalker &walker,
                         const SchemaResolver &resolver,
                         const SchemaDependencyCallback &callback,
                         const SchemaDependenciesOptions &options = {})
    -> std::uint64_t;

} // namespace sourcemeta::core

#endif
