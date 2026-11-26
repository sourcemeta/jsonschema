#include <sourcemeta/blaze/canonicalizer.h>
#include <sourcemeta/blaze/codegen.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/uri.h>

#include <algorithm>     // std::ranges::sort
#include <cassert>       // assert
#include <functional>    // std::reference_wrapper
#include <unordered_map> // std::unordered_map
#include <unordered_set> // std::unordered_set
#include <vector>        // std::vector

#include "codegen_default_compiler.h"

namespace {

auto is_validation_subschema(
    const sourcemeta::core::SchemaFrame &frame,
    const sourcemeta::core::SchemaFrame::Location &location,
    const sourcemeta::core::SchemaWalker &walker,
    const sourcemeta::core::SchemaResolver &resolver) -> bool {
  if (!location.parent.has_value()) {
    return false;
  }

  const auto &parent{location.parent.value()};
  if (parent.size() >= location.pointer.size()) {
    return false;
  }

  const auto &keyword_token{location.pointer.at(parent.size())};
  if (!keyword_token.is_property()) {
    return false;
  }

  const auto parent_location{frame.traverse(parent)};
  if (!parent_location.has_value()) {
    return false;
  }

  const auto &vocabularies{
      frame.vocabularies(parent_location.value().get(), resolver)};
  const auto &walker_result{walker(keyword_token.to_property(), vocabularies)};
  using Type = sourcemeta::core::SchemaKeywordType;
  if (walker_result.type == Type::ApplicatorValueTraverseAnyPropertyKey ||
      walker_result.type == Type::ApplicatorValueTraverseAnyItem ||
      walker_result.type == Type::ApplicatorValueInPlaceNegate) {
    return true;
  }

  return is_validation_subschema(frame, parent_location.value().get(), walker,
                                 resolver);
}

using Subschemas = std::vector<
    std::reference_wrapper<const sourcemeta::core::SchemaFrame::Location>>;

/// Where the document holds a subschema, by the place it sits at
using Subschemata = std::unordered_map<
    sourcemeta::core::WeakPointer,
    std::reference_wrapper<const sourcemeta::core::SchemaFrame::Location>,
    sourcemeta::core::WeakPointer::Hasher,
    sourcemeta::core::WeakPointer::Comparator>;

auto index_subschemas(const Subschemas &subschemas) -> Subschemata {
  Subschemata result;
  result.reserve(subschemas.size());
  for (const auto &candidate : subschemas) {
    result.emplace(candidate.get().pointer, candidate);
  }

  return result;
}

/// The innermost subschema a given one sits inside, or nothing when it is the
/// document itself. Walking out a token at a time stops at the first place the
/// document holds a subschema, which is the innermost one, so what this costs
/// is how deep the place sits rather than how many subschemas there are
auto enclosing_subschema(const Subschemata &subschemata,
                         const sourcemeta::core::WeakPointer &pointer)
    -> const sourcemeta::core::SchemaFrame::Location * {
  auto current{pointer};
  while (!current.empty()) {
    current.pop_back();
    const auto match{subschemata.find(current)};
    if (match != subschemata.cend()) {
      return &match->second.get();
    }
  }

  return nullptr;
}

/// Whether the keyword leading from one subschema to another says only what an
/// instance must satisfy, never what it looks like. Negating a subschema is
/// one of these: the only negation that generates anything at all is the one a
/// node nothing satisfies is written as, and what that node generates names
/// nothing of the subschema it negates
auto edge_only_validates(const sourcemeta::core::SchemaFrame &frame,
                         const sourcemeta::core::SchemaWalker &walker,
                         const sourcemeta::core::SchemaResolver &resolver,
                         const sourcemeta::core::SchemaFrame::Location &parent,
                         const sourcemeta::core::SchemaFrame::Location &child)
    -> bool {
  const auto &token{child.pointer.at(parent.pointer.size())};
  if (!token.is_property()) {
    return false;
  }

  const auto &vocabularies{frame.vocabularies(parent, resolver)};
  using Type = sourcemeta::core::SchemaKeywordType;
  switch (walker(token.to_property(), vocabularies).type) {
    case Type::ApplicatorValueTraverseAnyItem:
    case Type::ApplicatorValueTraverseAnyPropertyKey:
    case Type::ApplicatorValueInPlaceNegate:
      return true;
    default:
      return false;
  }
}

/// Whether the keyword leading from one subschema to another reserves a place
/// for it rather than putting it to an instance. A document keeps whatever only
/// a reference reaches in a place of that kind, and an instance never meets
/// what sits there
auto edge_reserves_place(const sourcemeta::core::SchemaFrame &frame,
                         const sourcemeta::core::SchemaWalker &walker,
                         const sourcemeta::core::SchemaResolver &resolver,
                         const sourcemeta::core::SchemaFrame::Location &parent,
                         const sourcemeta::core::SchemaFrame::Location &child)
    -> bool {
  const auto &token{child.pointer.at(parent.pointer.size())};
  if (!token.is_property()) {
    return false;
  }

  const auto &vocabularies{frame.vocabularies(parent, resolver)};
  return walker(token.to_property(), vocabularies).type ==
         sourcemeta::core::SchemaKeywordType::LocationMembers;
}

/// What an edge of the document is: whether what spells it says only what an
/// instance must satisfy, and whether what spells it reserves a place rather
/// than putting what it reaches to an instance
struct Edge {
  bool validates;
  bool reserved;
};

/// Every edge of the document, by where the edge sits
using Edges = std::unordered_map<sourcemeta::core::WeakPointer, Edge,
                                 sourcemeta::core::WeakPointer::Hasher,
                                 sourcemeta::core::WeakPointer::Comparator>;

auto edges(const sourcemeta::core::SchemaFrame &frame,
           const sourcemeta::core::SchemaWalker &walker,
           const sourcemeta::core::SchemaResolver &resolver,
           const Subschemas &subschemas, const Subschemata &subschemata)
    -> Edges {
  Edges result;
  result.reserve(subschemas.size());
  for (const auto &candidate : subschemas) {
    const auto &subschema{candidate.get()};
    const auto *node{enclosing_subschema(subschemata, subschema.pointer)};
    if (node != nullptr) {
      result.emplace(subschema.pointer,
                     Edge{.validates = edge_only_validates(
                              frame, walker, resolver, *node, subschema),
                          .reserved = edge_reserves_place(
                              frame, walker, resolver, *node, subschema)});
    }
  }

  return result;
}

using Nodes = std::unordered_set<sourcemeta::core::WeakPointer,
                                 sourcemeta::core::WeakPointer::Hasher,
                                 sourcemeta::core::WeakPointer::Comparator>;

/// A reference of the document that says what an instance looks like, as the
/// edge it sits in and the subschema it arrives at
struct Arrival {
  sourcemeta::core::WeakPointer edge;
  sourcemeta::core::WeakPointer target;
};

/// Where the references of a document lead: the subschemas a reference names
/// at all, and the arrivals that do more than validate what they reach
struct References {
  std::vector<Arrival> describing;
  Nodes named;
};

/// Record where a reference arrives: a subschema a reference names at all,
/// and, where the edge the reference sits in does more than validate what it
/// reaches, a subschema the document describes
auto arrive(const Edges &edges, const sourcemeta::core::WeakPointer &source,
            const sourcemeta::core::WeakPointer &target, References &result)
    -> void {
  result.named.insert(target);

  // A reference sits inside the subschema that stands for the edge, as the
  // keyword that spells it
  auto pointer{sourcemeta::core::to_pointer(source)};
  pointer.pop_back();
  const auto edge{sourcemeta::core::to_weak_pointer(pointer)};
  const auto match{edges.find(edge)};
  if (match != edges.cend() && match->second.validates) {
    return;
  }

  result.describing.push_back(Arrival{
      .edge = match == edges.cend() ? sourcemeta::core::EMPTY_WEAK_POINTER
                                    : match->first,
      .target = target});
}

auto references(const sourcemeta::core::SchemaFrame &frame, const Edges &edges)
    -> References {
  References result;
  frame.for_each_reference_into(
      sourcemeta::core::EMPTY_WEAK_POINTER,
      [&frame, &edges, &result](
          const sourcemeta::core::SchemaReferenceType type,
          const sourcemeta::core::WeakPointer &source,
          const sourcemeta::core::SchemaFrame::Reference &reference) -> void {
        const auto destination{frame.traverse(reference.destination)};
        if (destination.has_value()) {
          arrive(edges, source, destination.value().get().pointer, result);
        }

        // A reference that settles while an instance is being evaluated
        // reaches whichever anchor of its name the evaluation path leads to,
        // and what generates for such a reference stands for all of them at
        // once, so the reference arrives at every one
        if (type != sourcemeta::core::SchemaReferenceType::Dynamic ||
            !reference.fragment.has_value()) {
          return;
        }

        frame.for_each_anchor(
            sourcemeta::core::SchemaReferenceType::Dynamic,
            [&edges, &source, &reference, &result](
                const std::string_view uri,
                const sourcemeta::core::SchemaFrame::Location &anchor) -> void {
              const sourcemeta::core::URI identifier{
                  sourcemeta::core::JSON::String{uri}};
              const auto fragment{identifier.fragment()};
              if (fragment.has_value() &&
                  fragment.value() == reference.fragment.value()) {
                arrive(edges, source, anchor.pointer, result);
              }
            });
      });

  return result;
}

/// The node a subschema belongs to: itself where a reference names it, and
/// otherwise the innermost node holding it. The document is a node of its
/// own, so every subschema belongs to one, and walking out a token at a time
/// arrives at the innermost of them
auto owner(const Nodes &nodes, const sourcemeta::core::WeakPointer &pointer)
    -> sourcemeta::core::WeakPointer {
  auto current{pointer};
  while (!current.empty()) {
    if (nodes.contains(current)) {
      return current;
    }

    current.pop_back();
  }

  return sourcemeta::core::EMPTY_WEAK_POINTER;
}

/// The subschemas that say what an instance looks like. While a dialect keeps
/// its subschemas nested, the keywords above one are enough to tell what it is
/// for, and a reference that names such a subschema changes nothing about
/// that. Once a dialect lifts every subschema into one container, nothing sits
/// under anything, and what says it is the edges walked to get there: the
/// document says what an instance looks like, and so does whatever a chain of
/// references reaches from it without an edge that only validates along the
/// way. What sits inside such a subschema says what it says, as nothing that
/// generates can name it otherwise
auto describing(const Subschemas &subschemas, const Edges &edges,
                const References &references) -> Nodes {
  Nodes nodes;
  nodes.insert(sourcemeta::core::EMPTY_WEAK_POINTER);
  for (const auto &target : references.named) {
    // What stands on its own is what the document keeps where only a
    // reference reaches it
    const auto match{edges.find(target)};
    if (match != edges.cend() && match->second.reserved) {
      nodes.insert(target);
    }
  }

  std::unordered_map<sourcemeta::core::WeakPointer,
                     std::vector<sourcemeta::core::WeakPointer>,
                     sourcemeta::core::WeakPointer::Hasher,
                     sourcemeta::core::WeakPointer::Comparator>
      outgoing;
  for (const auto &arrival : references.describing) {
    outgoing[owner(nodes, arrival.edge)].push_back(
        owner(nodes, arrival.target));
  }

  Nodes reached;
  std::vector<sourcemeta::core::WeakPointer> pending;
  const auto mark{
      [&reached, &pending](const sourcemeta::core::WeakPointer &node) -> void {
        if (reached.insert(node).second) {
          pending.push_back(node);
        }
      }};

  mark(sourcemeta::core::EMPTY_WEAK_POINTER);
  while (!pending.empty()) {
    const auto node{pending.back()};
    pending.pop_back();
    const auto match{outgoing.find(node)};
    if (match == outgoing.cend()) {
      continue;
    }

    for (const auto &target : match->second) {
      mark(target);
    }
  }

  Nodes result;
  for (const auto &candidate : subschemas) {
    const auto &subschema{candidate.get()};
    if (reached.contains(owner(nodes, subschema.pointer))) {
      result.insert(subschema.pointer);
    }
  }

  return result;
}

// The dialect a schema declares, falling back to the given default. Unlike the
// equivalent helper in bundling, this one hands back a copy rather than a view,
// as canonicalisation goes on to rewrite the very document it reads from
auto declared_dialect(const sourcemeta::core::JSON &schema,
                      const std::string_view default_dialect)
    -> sourcemeta::core::JSON::String {
  if (!schema.is_object()) {
    return sourcemeta::core::JSON::String{default_dialect};
  }

  const auto *dialect{schema.try_at("$schema")};
  return (dialect != nullptr && dialect->is_string())
             ? dialect->to_string()
             : sourcemeta::core::JSON::String{default_dialect};
}

} // anonymous namespace

namespace sourcemeta::blaze {

auto compile(const sourcemeta::core::JSON &input,
             const sourcemeta::core::SchemaWalker &walker,
             const sourcemeta::core::SchemaResolver &resolver,
             const CodegenCompiler &compiler,
             const std::string_view default_dialect,
             const std::string_view default_id) -> CodegenIRResult {
  // --------------------------------------------------------------------------
  // (1) Bundle the schema to resolve external references
  // --------------------------------------------------------------------------

  sourcemeta::core::SchemaBundleOptions bundle_options;
  bundle_options.mode = sourcemeta::core::SchemaBundleOptions::Mode::References;
  auto schema{sourcemeta::core::schema_bundle(
      input, walker, resolver, default_dialect, default_id, bundle_options)};

  // --------------------------------------------------------------------------
  // (2) Canonicalize the schema for easier analysis
  // --------------------------------------------------------------------------

  // Canonicalisation can collapse an unsatisfiable schema into a boolean,
  // which has no room for the dialect the document declares, so hold on to
  // that dialect for the framing that follows
  const auto dialect{declared_dialect(schema, default_dialect)};

  sourcemeta::blaze::canonicalize(schema, walker, resolver, dialect,
                                  default_id);

  // --------------------------------------------------------------------------
  // (3) Frame the resulting schema with instance location information
  // --------------------------------------------------------------------------

  sourcemeta::core::SchemaFrame frame{
      sourcemeta::core::SchemaFrame::Mode::References,
      schema,
      walker,
      resolver,
      dialect,
      default_id};

  // --------------------------------------------------------------------------
  // (4) Convert every subschema into a code generation object
  // --------------------------------------------------------------------------

  std::unordered_set<sourcemeta::core::WeakPointer,
                     sourcemeta::core::WeakPointer::Hasher,
                     sourcemeta::core::WeakPointer::Comparator>
      visited;
  Subschemas subschemas;
  frame.for_each_subschema(
      [&subschemas](const sourcemeta::core::SchemaFrame::Location &location)
          -> void { subschemas.emplace_back(location); });
  const auto subschemata{index_subschemas(subschemas)};
  const auto document_edges{
      edges(frame, walker, resolver, subschemas, subschemata)};
  const auto reached{describing(subschemas, document_edges,
                                references(frame, document_edges))};
  CodegenIRResult result;
  frame.for_each_subschema(
      [&](const sourcemeta::core::SchemaFrame::Location &location) -> void {
        // Framing may report resource twice or more given default identifiers
        // and nested resources
        const auto [visited_iterator, inserted] =
            visited.insert(location.pointer);
        if (!inserted) {
          return;
        }

        // Skip subschemas under validation-only keywords that do not contribute
        // to the type structure (like `contains`)
        if (is_validation_subschema(frame, location, walker, resolver)) {
          return;
        }

        // A dialect that lifts its subschemas leaves none of them under the
        // keyword that put them there, so what carries a type is what a chain
        // of references reaches without going through such a keyword
        if (!reached.contains(location.pointer)) {
          return;
        }

        const auto &subschema{sourcemeta::core::get(schema, location.pointer)};
        result.push_back(
            compiler(schema, frame, location, resolver, subschema));
      });

  // --------------------------------------------------------------------------
  // (5) Sort entries so that dependencies come before dependents
  // --------------------------------------------------------------------------

  std::ranges::sort(
      result,
      [](const CodegenIREntity &left, const CodegenIREntity &right) -> bool {
        return std::visit(
                   [](const auto &entry) -> auto { return entry.pointer; },
                   right) <
               std::visit(
                   [](const auto &entry) -> auto { return entry.pointer; },
                   left);
      });

  return result;
}

} // namespace sourcemeta::blaze
