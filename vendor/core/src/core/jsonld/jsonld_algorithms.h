#ifndef SOURCEMETA_CORE_JSONLD_ALGORITHMS_H_
#define SOURCEMETA_CORE_JSONLD_ALGORITHMS_H_

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonld.h>
#include <sourcemeta/core/jsonpointer.h>

#include <cstddef>    // std::size_t
#include <functional> // std::less
#include <map>        // std::map
#include <memory>     // std::shared_ptr
#include <optional>   // std::optional
#include <vector>     // std::vector

namespace sourcemeta::core {

struct TermDefinition {
  std::optional<JSON::String> iri;
  std::optional<JSON::String> type_mapping;
  std::vector<JSON::String> container;
  std::optional<JSON::String> language;
  bool has_language{false};
  std::optional<JSON::String> direction;
  bool has_direction{false};
  std::optional<JSON> context;
  std::optional<JSON::String> context_base;
  std::optional<JSON::String> index;
  std::optional<JSON::String> index_iri;
  std::optional<JSON::String> nest;
  bool reverse{false};
  bool prefix{false};
  bool is_protected{false};
};

struct ActiveContext {
  std::map<JSON::String, TermDefinition, std::less<>> terms;
  std::optional<JSON::String> base;
  std::optional<JSON::String> vocabulary;
  std::optional<JSON::String> default_language;
  std::optional<JSON::String> default_direction;
  std::shared_ptr<ActiveContext> previous;
  // Whether compaction may relativise absolute IRIs against the base IRI.
  bool compact_to_relative{true};
};

// The mutable state shared by the expansion algorithms for the duration of a
// single top-level expansion.
struct ExpansionState {
  // Used to load remote contexts. The chain detects recursive inclusion.
  const JSONLDResolver *resolver{nullptr};
  std::vector<JSON::String> remote_context_chain;
  std::optional<JSON::String> document_base;
  // When a scoped context is processed after the fact, remote references in it
  // resolve against the URL of the document that defined the term.
  std::optional<JSON::String> context_base_override;
  // Protected-term state for the context currently being processed.
  bool context_protected{false};
  bool protected_override{false};
  bool processing_1_0{false};

  // Remote context references resolve against the URL of the document that
  // contains them: the current remote context if any, otherwise the input
  // document. This is distinct from the active context base, which @base
  // mutates.
  [[nodiscard]] auto context_resolution_base() const
      -> std::optional<JSON::String> {
    if (!this->remote_context_chain.empty()) {
      return this->remote_context_chain.back();
    }
    if (this->context_base_override.has_value()) {
      return this->context_base_override;
    }
    return this->document_base;
  }
};

// Tracks, while a context is being processed, which terms have been fully
// defined (true) versus are still being defined (false, used to detect cycles).
using DefinedTerms = std::map<JSON::String, bool>;

// IRI Expansion (JSON-LD 1.1 API Section 5.2)
auto expand_iri(ExpansionState &state, ActiveContext &active_context,
                const JSON::String &value, const bool document_relative,
                const bool vocabulary, const JSON *const local_context,
                DefinedTerms *const defined, const WeakPointer &context_pointer)
    -> std::optional<JSON::String>;

// Create Term Definition (JSON-LD 1.1 API Section 5.1.1)
auto create_term_definition(ExpansionState &state,
                            ActiveContext &active_context,
                            const JSON &local_context, const JSON::String &term,
                            DefinedTerms &defined,
                            const WeakPointer &context_pointer) -> void;

// Context Processing (JSON-LD 1.1 API Section 5.1)
auto process_context(ExpansionState &state, ActiveContext &active_context,
                     const JSON &local_context, const WeakPointer &pointer,
                     const bool propagate = true) -> void;

// Value Expansion (JSON-LD 1.1 API Section 5.3.3)
auto expand_value(ExpansionState &state, ActiveContext &active_context,
                  const std::optional<JSON::String> &active_property,
                  const JSON &value) -> JSON;

// Expansion (JSON-LD 1.1 API Section 5.1.2)
auto expand(ExpansionState &state, ActiveContext &active_context,
            const std::optional<JSON::String> &active_property,
            const JSON &element, const WeakPointer &pointer) -> JSON;

// Inverse Context Creation (JSON-LD 1.1 API Section 4.3.1). The inverse context
// is represented as a JSON map of IRI to container to type/language to term.
auto create_inverse_context(const ActiveContext &active_context) -> JSON;

// IRI Compaction (JSON-LD 1.1 API Section 6.2). A null value is signalled by
// a null pointer.
auto compact_iri(const ActiveContext &active_context,
                 const JSON &inverse_context, const JSON::String &variable,
                 const JSON *const value, const bool vocabulary,
                 const bool reverse) -> JSON::String;

// Value Compaction (JSON-LD 1.1 API Section 6.3)
auto compact_value(const ActiveContext &active_context,
                   const JSON &inverse_context,
                   const std::optional<JSON::String> &active_property,
                   const JSON &value) -> JSON;

// Compaction (JSON-LD 1.1 API Section 6.1)
auto compact(ExpansionState &state, const ActiveContext &active_context,
             const JSON &inverse_context,
             const std::optional<JSON::String> &active_property,
             const JSON &element, const bool compact_arrays) -> JSON;

// Holds the running state for blank node identifier generation, so that
// identifiers in the source are consistently relabelled without collisions.
struct BlankNodeState {
  std::size_t counter{0};
  std::map<JSON::String, JSON::String, std::less<>> identifiers;
};

// Generate Blank Node Identifier (JSON-LD 1.1 API Section 7.4). A null value is
// signalled by an empty optional. The view, when present, only needs to outlive
// the call, as the running state stores an owned copy.
auto generate_blank_node_identifier(
    BlankNodeState &state, const std::optional<JSON::StringView> &identifier)
    -> JSON::String;

// Node Map Generation (JSON-LD 1.1 API Section 7.2). The node map is a JSON map
// of graph name to a map of subject identifier to node object. The active
// subject is a subject identifier string, or a node object when a reverse
// property relationship is being processed, or a null pointer. A null active
// property is signalled by an empty optional, and the list accumulator by a
// null pointer. The active property view only needs to outlive the call.
auto generate_node_map(BlankNodeState &state, JSON &node_map,
                       const JSON &element, const JSON::StringView active_graph,
                       const JSON *active_subject,
                       const std::optional<JSON::StringView> &active_property,
                       JSON *list) -> void;

// Flattening (JSON-LD 1.1 API Section 7.1). Returns the flattened document in
// expanded form.
auto flatten(BlankNodeState &state, const JSON &element) -> JSON;

} // namespace sourcemeta::core

#endif
