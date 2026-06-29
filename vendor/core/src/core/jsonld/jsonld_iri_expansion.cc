#include "jsonld_algorithms.h"
#include "jsonld_keywords.h"

#include <sourcemeta/core/uri.h>

#include <optional> // std::optional

namespace sourcemeta::core {

// IRI Expansion (JSON-LD 1.1 API Section 5.2)
auto expand_iri(ExpansionState &state, ActiveContext &active_context,
                const JSON::String &value, const bool document_relative,
                const bool vocabulary, const JSON *const local_context,
                DefinedTerms *const defined, const WeakPointer &context_pointer)
    -> std::optional<JSON::String> {
  if (is_keyword(value)) {
    return value;
  }

  if (has_keyword_form(value)) {
    return std::nullopt;
  }

  if (local_context != nullptr && defined != nullptr &&
      local_context->is_object() && local_context->defines(value)) {
    const auto iterator{defined->find(value)};
    if (iterator == defined->cend() || !iterator->second) {
      create_term_definition(state, active_context, *local_context, value,
                             *defined, context_pointer);
    }
  }

  const auto term{active_context.terms.find(value)};
  if (term != active_context.terms.cend() && term->second.iri.has_value() &&
      is_keyword(term->second.iri.value())) {
    return term->second.iri;
  }

  if (vocabulary && term != active_context.terms.cend()) {
    return term->second.iri;
  }

  if (value.find(':', 1) != JSON::String::npos) {
    // The term has the form of a compact IRI, so split at the first colon.
    const auto colon{value.find(':')};
    const auto prefix{value.substr(0, colon)};
    const auto suffix{value.substr(colon + 1)};
    if (prefix == "_" || suffix.starts_with("//")) {
      return value;
    }

    if (local_context != nullptr && defined != nullptr &&
        local_context->is_object() && local_context->defines(prefix)) {
      const auto iterator{defined->find(prefix)};
      if (iterator == defined->cend() || !iterator->second) {
        create_term_definition(state, active_context, *local_context, prefix,
                               *defined, context_pointer);
      }
    }

    const auto definition{active_context.terms.find(prefix)};
    if (definition != active_context.terms.cend() &&
        definition->second.iri.has_value() && definition->second.prefix) {
      return definition->second.iri.value() + suffix;
    }

    // The value is only already an IRI when its prefix is a valid scheme.
    // Otherwise it is resolved against the vocabulary or document base below.
    if (URI::is_scheme(prefix)) {
      return value;
    }
  }

  if (vocabulary && active_context.vocabulary.has_value()) {
    return active_context.vocabulary.value() + value;
  }

  if (document_relative && active_context.base.has_value()) {
    return URI::from_iri(value)
        .resolve_from(URI::from_iri(active_context.base.value()))
        .recompose();
  }

  return value;
}

} // namespace sourcemeta::core
