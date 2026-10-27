#ifndef SOURCEMETA_CORE_JSONLD_ALGORITHMS_H_
#define SOURCEMETA_CORE_JSONLD_ALGORITHMS_H_

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonld.h>
#include <sourcemeta/core/jsonpointer.h>

#include <cstddef>     // std::size_t
#include <functional>  // std::less
#include <map>         // std::map
#include <memory>      // std::shared_ptr
#include <optional>    // std::optional
#include <string_view> // std::string_view
#include <utility>     // std::move
#include <vector>      // std::vector

namespace sourcemeta::core {

// A blank node identifier begins with "_:" (JSON-LD 1.1 Section 3.5). A
// label-less "_:" still counts, so that processing algorithms relabel every
// blank-flavored identifier instead of leaking one as an IRI. Expanded form
// validation additionally requires a label and keeps its own stricter check.
inline auto is_blank_node(const std::string_view value) -> bool {
  return value.starts_with("_:");
}

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
  // Where what the scoped context says wrong is reported when it is processed
  // after the fact, which is where it was written rather than wherever the
  // term later comes into use
  Pointer context_location;
  // Whether the input spells out the entries of the scoped context at that
  // location, as opposed to only the reference that brought them in
  bool context_authored{false};
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
  // Whether the document is processed under the JSON-LD 1.0 processing mode,
  // which the compaction container selection consults without access to the
  // expansion state.
  bool processing_1_0{false};
};

// The mutable state shared by the expansion algorithms for the duration of a
// single top-level expansion.
// RAII counter that bounds the depth of the mutually recursive expansion and
// compaction algorithms, decrementing on every exit path
struct NestingDepthScope {
  std::size_t &counter;
  explicit NestingDepthScope(std::size_t &value) : counter{value} {
    this->counter += 1;
  }
  ~NestingDepthScope() { this->counter -= 1; }
  NestingDepthScope(const NestingDepthScope &) = delete;
  auto operator=(const NestingDepthScope &) -> NestingDepthScope & = delete;
  NestingDepthScope(NestingDepthScope &&) = delete;
  auto operator=(NestingDepthScope &&) -> NestingDepthScope & = delete;
};

struct ExpansionState {
  // Bounds the expansion and compaction recursion so a deeply nested document
  // cannot overflow the stack on attacker-controlled input. The bound is
  // conservative so that it holds on platforms with a small default stack such
  // as Windows, and is still far above any realistic nesting
  static constexpr std::size_t MAXIMUM_DEPTH{100};
  std::size_t depth{0};

  // Bounds the remote context chain, which the resolver grows by one for every
  // reference it answers with, and is therefore attacker-controlled the same
  // way the nesting depth is. A processor is free to set this limit (JSON-LD
  // 1.1 API Section 5.1 step 5.2.3)
  static constexpr std::size_t MAXIMUM_REMOTE_CONTEXTS{32};

  // Used to load remote contexts. The chain detects recursive inclusion.
  const JSONLDResolver *resolver{nullptr};
  // Used to report the input that expansion discards.
  const JSONLDExpandDropCallback *drop_callback{nullptr};
  // Whether a reference already in the chain is an error. A scoped context is
  // validated against the contexts that are loading it, where meeting one of
  // them again is the recursion the validation must not follow (JSON-LD 1.1
  // API Section 5.1 step 5.2.2)
  bool validate_scoped_context{true};
  std::vector<JSON::String> remote_context_chain;
  // The `@context` entry of every remote context already dereferenced in this
  // expansion, keyed by the reference that loaded it. Step 5.2.4: "If context
  // was previously dereferenced, then the processor MUST NOT do a further
  // dereference, and context is set to the previously established internal
  // representation: set context document to the previously dereferenced
  // document, and set loaded context to the value of the @context entry from
  // the document in context document", whose note adds that "Only the
  // @context entry need be retained" (JSON-LD 1.1 API Section 5.1 step 5.2.4)
  std::map<JSON::String, JSON, std::less<>> remote_documents;
  std::optional<JSON::String> document_base;
  // When a scoped context is processed after the fact, remote references in it
  // resolve against the URL of the document that defined the term.
  std::optional<JSON::String> context_base_override;
  // Where to report what the context currently being processed says wrong when
  // the input does not spell out its entries: one fetched by reference, one
  // merged in from an import, and the expansion context the caller supplies
  // are each named by what brought them rather than by their own entries.
  // Absent while the input's own context is being processed
  std::optional<Pointer> foreign_context_location;
  // While a context merged from an @import is being processed, the entries the
  // input itself spells out. Those terms are located by their own scoped
  // context, unlike the ones the imported document contributed
  const JSON *input_context{nullptr};
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

// RAII guard that silences drop reporting for the duration of a diagnostic
// re-expansion, restoring it on every exit path. Locating an error must not
// change what is reported, and whatever such a re-expansion discards was
// already reported by the expansion that ran into the error
struct SuspendedDropReportingScope {
  ExpansionState &state;
  const JSONLDExpandDropCallback *previous;

  explicit SuspendedDropReportingScope(ExpansionState &value)
      : state{value}, previous{value.drop_callback} {
    this->state.drop_callback = nullptr;
  }

  ~SuspendedDropReportingScope() { this->state.drop_callback = this->previous; }

  SuspendedDropReportingScope(const SuspendedDropReportingScope &) = delete;
  auto operator=(const SuspendedDropReportingScope &)
      -> SuspendedDropReportingScope & = delete;
  SuspendedDropReportingScope(SuspendedDropReportingScope &&) = delete;
  auto operator=(SuspendedDropReportingScope &&)
      -> SuspendedDropReportingScope & = delete;
};

// RAII guard for where the context being processed reports what it says wrong,
// restoring the enclosing origin, and which of its entries the input itself
// spells out, on every exit path
struct ForeignContextScope {
  ExpansionState &state;
  std::optional<Pointer> previous_location;
  const JSON *previous_input;

  ForeignContextScope(ExpansionState &value, std::optional<Pointer> location,
                      const JSON *const input = nullptr)
      : state{value},
        previous_location{std::move(value.foreign_context_location)},
        previous_input{value.input_context} {
    this->state.foreign_context_location = std::move(location);
    this->state.input_context = input;
  }

  ~ForeignContextScope() {
    this->state.foreign_context_location = std::move(this->previous_location);
    this->state.input_context = this->previous_input;
  }

  ForeignContextScope(const ForeignContextScope &) = delete;
  auto operator=(const ForeignContextScope &) -> ForeignContextScope & = delete;
  ForeignContextScope(ForeignContextScope &&) = delete;
  auto operator=(ForeignContextScope &&) -> ForeignContextScope & = delete;
};

// A scoped context is processed with the protected override in force, and what
// was in force before comes back whichever way that processing ends
struct ProtectedOverrideScope {
  bool &flag;
  const bool previous;

  explicit ProtectedOverrideScope(bool &value) : flag{value}, previous{value} {
    this->flag = true;
  }

  ~ProtectedOverrideScope() { this->flag = this->previous; }

  ProtectedOverrideScope(const ProtectedOverrideScope &) = delete;
  auto operator=(const ProtectedOverrideScope &)
      -> ProtectedOverrideScope & = delete;
  ProtectedOverrideScope(ProtectedOverrideScope &&) = delete;
  auto operator=(ProtectedOverrideScope &&)
      -> ProtectedOverrideScope & = delete;
};

// The chain of remote contexts a document loaded through, which the one being
// processed joins for as long as it is being processed
struct RemoteContextScope {
  std::vector<JSON::String> &chain;

  RemoteContextScope(std::vector<JSON::String> &value,
                     const JSON::String &reference)
      : chain{value} {
    this->chain.push_back(reference);
  }

  ~RemoteContextScope() { this->chain.pop_back(); }

  RemoteContextScope(const RemoteContextScope &) = delete;
  auto operator=(const RemoteContextScope &) -> RemoteContextScope & = delete;
  RemoteContextScope(RemoteContextScope &&) = delete;
  auto operator=(RemoteContextScope &&) -> RemoteContextScope & = delete;
};

// RAII guard for the base that the remote references of the context being
// processed resolve against, restoring the enclosing one on every exit path
struct ContextBaseScope {
  ExpansionState &state;
  std::optional<JSON::String> previous;

  ContextBaseScope(ExpansionState &value, std::optional<JSON::String> base)
      : state{value}, previous{std::move(value.context_base_override)} {
    this->state.context_base_override = std::move(base);
  }

  ~ContextBaseScope() {
    this->state.context_base_override = std::move(this->previous);
  }

  ContextBaseScope(const ContextBaseScope &) = delete;
  auto operator=(const ContextBaseScope &) -> ContextBaseScope & = delete;
  ContextBaseScope(ContextBaseScope &&) = delete;
  auto operator=(ContextBaseScope &&) -> ContextBaseScope & = delete;
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

// Expansion (JSON-LD 1.1 API Section 5.1.2). A value taken from a container
// map does not begin a new scope, so the context a scope would be left for
// stays in force across it
auto expand(ExpansionState &state, ActiveContext &active_context,
            const std::optional<JSON::String> &active_property,
            const JSON &element, const WeakPointer &pointer,
            const bool from_map = false) -> JSON;

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
