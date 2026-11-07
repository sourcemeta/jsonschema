#include <sourcemeta/core/jsonschema.h>

#include "helpers.h"

#include <algorithm>     // std::ranges::all_of
#include <cassert>       // assert
#include <cstdint>       // std::uint64_t
#include <deque>         // std::deque
#include <functional>    // std::equal_to
#include <memory>        // std::unique_ptr, std::make_unique
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <unordered_set> // std::unordered_set
#include <utility>       // std::move
#include <vector>        // std::vector

namespace sourcemeta::core {

namespace {

auto is_skippable_metaschema_reference(const WeakPointer &pointer,
                                       const std::string &destination) -> bool {
  assert(!pointer.empty());
  assert(pointer.back().is_property());
  if (pointer.back().to_property() != "$schema") {
    return false;
  }

  return schema_is_official(destination);
}

using URISet = std::unordered_set<JSON::String, URIHash, std::equal_to<>>;

// A document that the walk read, beside the URI it asked for. A document also
// answers to whatever identifier it declares, but what the walk asked for is
// what the caller has to fetch, so that is what an edge reports
struct Fetched {
  SchemaResolverResult document;
  JSON::String identifier;
};

// What the walk carries from one document to the next
struct Walk {
  const SchemaWalker &walker;
  const SchemaResolver &resolver;
  SchemaResolverCache &cache;
  const SchemaDependencyCallback &callback;
  std::string_view default_dialect;
  std::uint64_t limit;
  std::uint64_t remaining;
  URISet accounted;
  const SchemaDependenciesOptions::AccountedPredicate &accounted_predicate;
  const SchemaDependenciesOptions::AccountedCallback &accounted_callback;
  std::vector<JSON::String> deferred;
  std::deque<Fetched> pending;
};

// Whatever the caller holds counts here too, and whatever this comes to
// account for is handed back, which is how several walks of one span agree on
// what has been read and what a reference still needs
auto is_accounted(const Walk &walk, const JSON::StringView identifier) -> bool {
  return walk.accounted.contains(identifier) ||
         (walk.accounted_predicate && walk.accounted_predicate(identifier));
}

auto account(Walk &walk, const JSON::StringView identifier) -> void {
  if (walk.accounted.emplace(identifier).second && walk.accounted_callback) {
    walk.accounted_callback(identifier);
  }
}

// Every frame that the walk constructs spends from the same allowance, as how
// many of them it ends up needing is a function of what the resolver hands
// back rather than of the schema the caller passed in. A frame that ran past
// what was left of the allowance threw rather than returned, so what it holds
// is always within it
//
// Only framing is held to that allowance. The caller set it for this
// operation, so the frame that ran out reports it rather than whatever was
// left by then, while a limit error raised by the caller's own resolver or
// callback carries an allowance of theirs and travels untouched
auto frame_within(Walk &walk, const JSON &schema,
                  const std::string_view default_id,
                  const SchemaFrame::Paths &paths,
                  const std::string_view default_base)
    -> std::unique_ptr<SchemaFrame> {
  try {
    auto frame{std::make_unique<SchemaFrame>(
        SchemaFrame::Mode::References, schema, walk.walker, walk.resolver,
        walk.default_dialect, default_id,
        SchemaFrame::IdentifierMode::Additional, paths, default_base,
        walk.remaining)};
    assert(frame->location_count() <= walk.remaining);
    walk.remaining -= frame->location_count();
    return frame;
  } catch (const SchemaFrameLimitError &) {
    throw SchemaFrameLimitError{walk.limit};
  }
}

// Reading a document takes knowing which dialect it is written against, so one
// that answers with nothing, or whose answer names a meta-schema that nothing
// can produce, is not one the walk can go on from
//
// A meta-schema that a document carries within itself counts here, as reading
// consults the document before the resolver. One that another document carries
// does not, and deliberately so. Such a meta-schema is a resource of the
// document holding it, and reaching it from elsewhere would take keeping an
// index of every resource of every document the walk has read, which is the
// one thing it goes out of its way not to do, as it lets each document go as
// soon as it is done with it. To have it reached, answer for that meta-schema
// from the resolver passed in
auto is_readable(const JSON &document, const SchemaResolver &resolver,
                 const std::string_view default_dialect) -> bool {
  try {
    return base_dialect(document, resolver, default_dialect).has_value();
  } catch (const SchemaUnknownBaseDialectError &) {
    return false;
  } catch (const SchemaResolutionError &) {
    return false;
  }
}

auto examine(Walk &walk, const SchemaFrame &frame,
             const std::string_view origin) -> void {
  // Every URI that a document the walk holds answers to is one the caller
  // never has to fetch of its own, whether the document declares it at the top
  // or carries it within itself
  frame.for_each_resource_uri(
      [&walk](const auto &uri) -> void { account(walk, uri); });

  frame.for_each_unresolved_reference([&walk, &frame,
                                       origin](const auto &pointer,
                                               const auto &reference) -> void {
    // We don't want to report official schemas, as we can expect
    // virtually all implementations to understand them out of the box
    if (is_skippable_metaschema_reference(pointer, reference.destination)) {
      return;
    }

    if (reference.base.empty()) {
      throw SchemaReferenceError("Could not resolve schema reference",
                                 reference.destination, to_pointer(pointer));
    }

    // If we can't find the destination but there is a base and we can
    // find the base, then we are facing an unresolved fragment. This comes
    // before the check below, as a document that the walk already accounts
    // for is still the one that has to hold what a reference into it names
    if (frame.traverse(reference.base).has_value()) {
      throw SchemaReferenceError("Could not resolve schema reference",
                                 reference.destination, to_pointer(pointer));
    }

    // Whatever the walk accounts for travels with the document it came from,
    // so a reference into it reaches nothing the caller has to fetch of its
    // own. This is also what keeps a cycle from going round for ever
    if (is_accounted(walk, reference.base)) {
      return;
    }

    auto remote{walk.resolver(reference.base)};
    if (!remote.has_value()) {
      // A document that the walk has not read yet may carry the resource
      // that this names, as a resource embedded in one document is reached
      // the same way as one linked from another. Whether this leads anywhere
      // is settled once there is nothing left to read rather than here
      walk.deferred.emplace_back(reference.base);
      return;
    }

    if (!remote.value().is_object() && !remote.value().is_boolean()) {
      throw SchemaReferenceError("The JSON document is not a valid JSON Schema",
                                 reference.base, to_pointer(pointer));
    }

    // A boolean schema has nowhere to hold a reference, so it needs no dialect
    // to be read against and nothing comes of reading it
    const auto standalone{remote.value().is_boolean()};
    if (!standalone &&
        !is_readable(remote.value(), walk.resolver, walk.default_dialect)) {
      throw SchemaReferenceError("The JSON document is not a valid JSON Schema",
                                 reference.base, to_pointer(pointer));
    }

    walk.callback({.origin = origin,
                   .pointer = pointer,
                   .destination = reference.base,
                   .document = remote.value()});
    account(walk, reference.base);

    // Official schemas can only reference other official schemas, so reading
    // them can never surface further dependencies
    if (standalone || schema_is_official(reference.base)) {
      return;
    }

    // What the walk takes over it also releases once it is done with, so the
    // cache hands the document over rather than keeping a copy of the closure
    // alive behind it. Nothing asks for this one again, as it is accounted for
    auto taken{walk.cache.extract(JSON::String{reference.base})};
    walk.pending.emplace_back(taken.empty() ? std::move(remote)
                                            : std::move(taken.mapped()),
                              JSON::String{reference.base});
  });
}

} // namespace

auto schema_dependencies(const JSON &schema, const SchemaWalker &walker,
                         const SchemaResolver &resolver,
                         const SchemaDependencyCallback &callback,
                         const SchemaDependenciesOptions &options)
    -> std::uint64_t {
  assert(callback);
  // Framing reaches for the resolver of its own accord, so a limit error the
  // caller's own resolver raises has to be told apart from one this
  // operation's allowance ran into
  SchemaResolverCache cache;
  const SchemaResolver bounded{resolver_within_allowance(resolver)};
  const SchemaResolver cached{resolver_with_cache(bounded, cache)};
  // Framing states these as preconditions rather than checking them, and a
  // path that names nothing otherwise fails deep inside the document rather
  // than at the call that got it wrong
  assert(
      std::ranges::all_of(options.paths, [&schema](const auto &path) -> bool {
        const auto *target{try_get(schema, path)};
        return target != nullptr &&
               (target->is_object() || target->is_boolean());
      }));

  Walk walk{.walker = walker,
            .resolver = cached,
            .cache = cache,
            .callback = callback,
            .default_dialect = options.default_dialect,
            .limit = options.max_locations,
            .remaining = options.max_locations,
            .accounted = {},
            .accounted_predicate = options.accounted,
            .accounted_callback = options.accounted_callback,
            .deferred = {},
            .pending = {}};

  try {
    {
      const auto frame{frame_within(walk, schema, options.default_id,
                                    options.paths, options.default_base)};

      // A document that declares no identifier of its own still answers to
      // wherever it was retrieved from, which framing files as a place within
      // the document rather than as a resource of it. Framing above has
      // already made a URI of this one, so there is no second way for it to
      // fail here
      if (!options.default_base.empty()) {
        account(walk,
                sourcemeta::core::URI::canonicalize(options.default_base));
      }

      examine(walk, *frame, frame->root());
    }

    // Reading a document can only ever add to what the walk accounts for, so
    // nothing that was put aside can come good until there is nothing left to
    // read, and once there is nothing left the answer cannot change again
    // Framing a document that the walk read from somewhere takes no paths, as
    // the whole of it is the schema
    static const SchemaFrame::Paths WHOLE_DOCUMENT{EMPTY_WEAK_POINTER};
    while (!walk.pending.empty()) {
      const auto fetched{std::move(walk.pending.front())};
      walk.pending.pop_front();
      const auto frame{frame_within(walk, fetched.document.value(),
                                    fetched.identifier, WHOLE_DOCUMENT,
                                    fetched.identifier)};
      examine(walk, *frame, fetched.identifier);
    }

    for (const auto &identifier : walk.deferred) {
      if (!is_accounted(walk, identifier)) {
        throw SchemaResolutionError(
            "Could not resolve the reference to an external schema",
            identifier);
      }
    }
  } catch (const ForeignFrameLimitError &foreign) {
    throw foreign.error;
  }

  return options.max_locations - walk.remaining;
}

} // namespace sourcemeta::core
