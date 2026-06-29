#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonld.h>
#include <sourcemeta/core/jsonpointer.h>

#include "jsonld_algorithms.h"
#include "jsonld_keywords.h"

#include <cassert>  // assert
#include <optional> // std::nullopt
#include <utility>  // std::move

namespace sourcemeta::core {

// Run the expansion algorithm on a top-level input document and normalise the
// result into the expanded document form (JSON-LD 1.1 API Section 5.1).
auto run_expansion(ExpansionState &state, ActiveContext &active_context,
                   const JSON &input) -> JSON {
  auto expanded{
      expand(state, active_context, std::nullopt, input, empty_weak_pointer)};

  // A top-level map containing only @graph is replaced by its value.
  if (expanded.is_object() && expanded.object_size() == 1 &&
      expanded.defines(KEYWORD_GRAPH, KEYWORD_GRAPH_HASH)) {
    expanded = expanded.at(KEYWORD_GRAPH, KEYWORD_GRAPH_HASH);
  }

  if (expanded.is_object()) {
    if (expanded.empty() || (expanded.object_size() == 1 &&
                             expanded.defines(KEYWORD_ID, KEYWORD_ID_HASH))) {
      return JSON::make_array();
    }
    auto result{JSON::make_array()};
    result.push_back(std::move(expanded));
    return result;
  }

  if (!expanded.is_array()) {
    return JSON::make_array();
  }

  return expanded;
}

// Set up the shared expansion state and initial active context from the public
// entry-point arguments.
auto initialise_expansion(const JSONLDResolver &resolver,
                          const JSON::StringView base_iri,
                          const JSONLDVersion version, ExpansionState &state,
                          ActiveContext &active_context) -> void {
  state.resolver = &resolver;
  state.processing_1_0 = version == JSONLDVersion::V1_0;
  if (!base_iri.empty()) {
    active_context.base = JSON::String{base_iri};
    state.document_base = JSON::String{base_iri};
  }
}

} // namespace sourcemeta::core

namespace sourcemeta::core {

auto jsonld_expand(const JSON &input, const JSON::StringView base_iri,
                   const JSONLDResolver &resolver, const JSONLDVersion version)
    -> JSON {
  ExpansionState state;
  ActiveContext active_context;
  initialise_expansion(resolver, base_iri, version, state, active_context);
  return run_expansion(state, active_context, input);
}

auto jsonld_expand(const JSON &input, const JSON &expand_context,
                   const JSON::StringView base_iri,
                   const JSONLDResolver &resolver, const JSONLDVersion version)
    -> JSON {
  ExpansionState state;
  ActiveContext active_context;
  initialise_expansion(resolver, base_iri, version, state, active_context);
  const auto &context{
      expand_context.is_object() &&
              expand_context.defines(KEYWORD_CONTEXT, KEYWORD_CONTEXT_HASH)
          ? expand_context.at(KEYWORD_CONTEXT, KEYWORD_CONTEXT_HASH)
          : expand_context};
  // The external expansion context is not part of the input document, so its
  // errors carry an empty pointer.
  process_context(state, active_context, context, empty_weak_pointer);
  return run_expansion(state, active_context, input);
}

auto jsonld_compact(const JSON &input, const JSON &context,
                    const JSON::StringView base_iri,
                    const JSONLDResolver &resolver, const JSONLDVersion version,
                    const bool compact_arrays, const bool compact_to_relative)
    -> JSON {
  // Compaction operates on input already in expanded document form. Callers
  // that hold a non-expanded document expand it first.
  assert(jsonld_is_expanded(input));

  ExpansionState state;
  ActiveContext active_context;
  initialise_expansion(resolver, base_iri, version, state, active_context);
  active_context.compact_to_relative = compact_to_relative;
  const auto &local_context{
      context.is_object() &&
              context.defines(KEYWORD_CONTEXT, KEYWORD_CONTEXT_HASH)
          ? context.at(KEYWORD_CONTEXT, KEYWORD_CONTEXT_HASH)
          : context};
  process_context(state, active_context, local_context, empty_weak_pointer);

  const auto inverse_context{create_inverse_context(active_context)};
  auto result{compact(state, active_context, inverse_context, std::nullopt,
                      input, compact_arrays)};

  // A top-level array becomes a node with a single @graph entry.
  if (result.is_array()) {
    if (result.empty()) {
      result = JSON::make_object();
    } else {
      const auto graph_alias{compact_iri(active_context, inverse_context,
                                         JSON::String{KEYWORD_GRAPH}, nullptr,
                                         true, false)};
      auto wrapper{JSON::make_object()};
      wrapper.assign_assume_new(graph_alias, std::move(result));
      result = std::move(wrapper);
    }
  }

  // The compacted document carries the context it was compacted against, unless
  // that context is empty.
  if ((local_context.is_object() && !local_context.empty()) ||
      (local_context.is_array() && !local_context.empty()) ||
      local_context.is_string()) {
    result.assign_assume_new(keyword_context(), JSON{local_context});
  }

  return result;
}

auto jsonld_flatten(const JSON &input) -> JSON {
  // Flattening operates on input already in expanded document form. Callers
  // that hold a non-expanded document expand it first.
  assert(jsonld_is_expanded(input));

  BlankNodeState state;
  return flatten(state, input);
}

auto jsonld_flatten(const JSON &input, const JSON &context,
                    const JSON::StringView base_iri,
                    const JSONLDResolver &resolver, const JSONLDVersion version,
                    const bool compact_arrays) -> JSON {
  // The flattened expanded output is compacted against the given context.
  return jsonld_compact(jsonld_flatten(input), context, base_iri, resolver,
                        version, compact_arrays);
}

} // namespace sourcemeta::core
