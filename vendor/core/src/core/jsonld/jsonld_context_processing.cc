#include "jsonld_algorithms.h"
#include "jsonld_keywords.h"

#include <sourcemeta/core/uri.h>

#include <memory>   // std::make_shared
#include <optional> // std::optional
#include <utility>  // std::move, std::pair
#include <vector>   // std::vector

namespace sourcemeta::core {

// Context Processing (JSON-LD 1.1 API Section 5.1)
auto process_context(ExpansionState &state, ActiveContext &active_context,
                     const JSON &local_context, const WeakPointer &pointer,
                     const bool propagate) -> void {
  std::vector<std::pair<const JSON *, WeakPointer>> contexts;
  if (local_context.is_array()) {
    std::size_t index{0};
    for (const auto &item : local_context.as_array()) {
      contexts.emplace_back(&item, pointer.concat(index));
      index += 1;
    }
  } else {
    contexts.emplace_back(&local_context, pointer);
  }

  // The @propagate flag is read once from a top-level map context, before the
  // contexts are processed (JSON-LD 1.1 API Section 5.1 steps 2 and 3). A
  // non-boolean value is reported per entry by the loop below.
  bool effective_propagate{propagate};
  if (local_context.is_object()) {
    if (const auto *propagate_entry{
            local_context.try_at(KEYWORD_PROPAGATE, KEYWORD_PROPAGATE_HASH)};
        propagate_entry != nullptr && propagate_entry->is_boolean()) {
      effective_propagate = propagate_entry->to_boolean();
    }
  }
  if (!effective_propagate && !active_context.previous) {
    auto snapshot{std::make_shared<ActiveContext>(active_context)};
    snapshot->previous = nullptr;
    active_context.previous = snapshot;
  }

  for (const auto &[entry_pointer, location] : contexts) {
    const auto &context{*entry_pointer};
    if (context.is_null()) {
      if (!state.protected_override) {
        for (const auto &entry : active_context.terms) {
          if (entry.second.is_protected) {
            throw JSONLDError("Invalid context nullification", location);
          }
        }
      }
      // Nullifying the context resets to the initial context, whose base is
      // the document base. Whether to relativise is a processing option rather
      // than context state, so it survives the reset.
      const bool relativise{active_context.compact_to_relative};
      active_context = ActiveContext{};
      active_context.base = state.document_base;
      active_context.compact_to_relative = relativise;
      continue;
    }

    if (context.is_string()) {
      auto reference{context.to_string()};
      const auto resolution_base{state.context_resolution_base()};
      if (resolution_base.has_value()) {
        reference = URI::from_iri(reference)
                        .resolve_from(URI::from_iri(resolution_base.value()))
                        .recompose();
      }
      for (const auto &loaded : state.remote_context_chain) {
        if (loaded == reference) {
          throw JSONLDError("Recursive context inclusion", location);
        }
      }
      if (state.resolver == nullptr || !*state.resolver) {
        throw JSONLDError("Loading remote context failed", location);
      }
      const auto document{(*state.resolver)(reference)};
      if (!document.has_value()) {
        throw JSONLDError("Loading remote context failed", location);
      }
      const auto *context_entry{
          document->is_object()
              ? document->try_at(KEYWORD_CONTEXT, KEYWORD_CONTEXT_HASH)
              : nullptr};
      if (context_entry == nullptr) {
        throw JSONLDError("Invalid remote context", location);
      }
      state.remote_context_chain.push_back(reference);
      try {
        // A loaded remote context is processed with the default propagation.
        process_context(state, active_context, *context_entry, location);
      } catch (...) {
        state.remote_context_chain.pop_back();
        throw;
      }
      state.remote_context_chain.pop_back();
      continue;
    }

    if (!context.is_object()) {
      throw JSONLDError("Invalid local context", location);
    }

    if (const auto *version{
            context.try_at(KEYWORD_VERSION, KEYWORD_VERSION_HASH)};
        version != nullptr &&
        (!version->is_real() || version->to_real() != 1.1)) {
      throw JSONLDError("Invalid @version value", location, {KEYWORD_VERSION});
    }
    if (state.processing_1_0 &&
        context.defines(KEYWORD_VERSION, KEYWORD_VERSION_HASH)) {
      throw JSONLDError("Processing mode conflict", location,
                        {KEYWORD_VERSION});
    }
    if (state.processing_1_0 &&
        (context.defines(KEYWORD_PROPAGATE, KEYWORD_PROPAGATE_HASH) ||
         context.defines(KEYWORD_IMPORT, KEYWORD_IMPORT_HASH) ||
         context.defines(KEYWORD_PROTECTED, KEYWORD_PROTECTED_HASH))) {
      throw JSONLDError("Invalid context entry", location);
    }

    if (const auto *propagate_entry{
            context.try_at(KEYWORD_PROPAGATE, KEYWORD_PROPAGATE_HASH)};
        propagate_entry != nullptr && !propagate_entry->is_boolean()) {
      throw JSONLDError("Invalid @propagate value", location,
                        {KEYWORD_PROPAGATE});
    }

    // @protected applies to imported terms too, so it is set before @import.
    const bool saved_protected{state.context_protected};
    if (const auto *protected_entry{
            context.try_at(KEYWORD_PROTECTED, KEYWORD_PROTECTED_HASH)}) {
      if (!protected_entry->is_boolean()) {
        throw JSONLDError("Invalid @protected value", location,
                          {KEYWORD_PROTECTED});
      }
      state.context_protected = protected_entry->to_boolean();
    }

    if (const auto *import_entry{
            context.try_at(KEYWORD_IMPORT, KEYWORD_IMPORT_HASH)}) {
      const auto &import{*import_entry};
      if (!import.is_string()) {
        throw JSONLDError("Invalid @import value", location, {KEYWORD_IMPORT});
      }
      auto reference{import.to_string()};
      const auto resolution_base{state.context_resolution_base()};
      if (resolution_base.has_value()) {
        reference = URI::from_iri(reference)
                        .resolve_from(URI::from_iri(resolution_base.value()))
                        .recompose();
      }
      if (state.resolver == nullptr || !*state.resolver) {
        throw JSONLDError("Loading remote context failed", location,
                          {KEYWORD_IMPORT});
      }
      const auto document{(*state.resolver)(reference)};
      if (!document.has_value()) {
        throw JSONLDError("Loading remote context failed", location,
                          {KEYWORD_IMPORT});
      }
      const auto *imported_context{
          document->is_object()
              ? document->try_at(KEYWORD_CONTEXT, KEYWORD_CONTEXT_HASH)
              : nullptr};
      if (imported_context == nullptr || !imported_context->is_object()) {
        throw JSONLDError("Invalid remote context", location, {KEYWORD_IMPORT});
      }
      if (imported_context->defines(KEYWORD_IMPORT, KEYWORD_IMPORT_HASH)) {
        throw JSONLDError("Invalid context entry", location, {KEYWORD_IMPORT});
      }
      // Merge the imported entries with the current ones, the current ones
      // overriding, and process the result as a single context.
      auto merged{JSON{*imported_context}};
      for (const auto &entry : context.as_object()) {
        if (!entry.key_equals(KEYWORD_IMPORT, KEYWORD_IMPORT_HASH)) {
          merged.assign(entry.first, entry.second);
        }
      }
      process_context(state, active_context, merged, location, propagate);
      state.context_protected = saved_protected;
      continue;
    }

    if (const auto *base_entry{
            state.remote_context_chain.empty()
                ? context.try_at(KEYWORD_BASE, KEYWORD_BASE_HASH)
                : nullptr}) {
      const auto &base{*base_entry};
      if (base.is_null()) {
        active_context.base = std::nullopt;
      } else if (!base.is_string()) {
        throw JSONLDError("Invalid base IRI", location, {KEYWORD_BASE});
      } else {
        const auto &base_string{base.to_string()};
        if (active_context.base.has_value()) {
          active_context.base =
              URI::from_iri(base_string)
                  .resolve_from(URI::from_iri(active_context.base.value()))
                  .recompose();
        } else if (URI::from_iri(base_string).is_absolute()) {
          active_context.base = base_string;
        } else {
          throw JSONLDError("Invalid base IRI", location, {KEYWORD_BASE});
        }
      }
    }

    if (const auto *vocabulary_entry{
            context.try_at(KEYWORD_VOCAB, KEYWORD_VOCAB_HASH)}) {
      const auto &vocabulary{*vocabulary_entry};
      if (vocabulary.is_null()) {
        active_context.vocabulary = std::nullopt;
      } else if (!vocabulary.is_string()) {
        throw JSONLDError("Invalid vocab mapping", location, {KEYWORD_VOCAB});
      } else {
        const auto &vocabulary_string{vocabulary.to_string()};
        // In 1.0, @vocab must be an absolute IRI or blank node identifier.
        if (state.processing_1_0 &&
            vocabulary_string.find(':') == JSON::String::npos) {
          throw JSONLDError("Invalid vocab mapping", location, {KEYWORD_VOCAB});
        }
        active_context.vocabulary =
            expand_iri(state, active_context, vocabulary_string, true, true,
                       nullptr, nullptr, empty_weak_pointer);
      }
    }

    if (const auto *language_entry{
            context.try_at(KEYWORD_LANGUAGE, KEYWORD_LANGUAGE_HASH)}) {
      const auto &language{*language_entry};
      if (language.is_null()) {
        active_context.default_language = std::nullopt;
      } else if (!language.is_string()) {
        throw JSONLDError("Invalid default language", location,
                          {KEYWORD_LANGUAGE});
      } else {
        active_context.default_language = language.to_string();
      }
    }

    if (const auto *direction_entry{
            context.try_at(KEYWORD_DIRECTION, KEYWORD_DIRECTION_HASH)}) {
      const auto &direction{*direction_entry};
      if (direction.is_null()) {
        active_context.default_direction = std::nullopt;
      } else if (!direction.is_string()) {
        throw JSONLDError("Invalid base direction", location,
                          {KEYWORD_DIRECTION});
      } else {
        const auto &direction_string{direction.to_string()};
        if (direction_string != "ltr" && direction_string != "rtl") {
          throw JSONLDError("Invalid base direction", location,
                            {KEYWORD_DIRECTION});
        }
        active_context.default_direction = direction_string;
      }
    }

    DefinedTerms defined;
    for (const auto &entry : context.as_object()) {
      const auto &name{entry.first};
      if (entry.key_equals(KEYWORD_BASE, KEYWORD_BASE_HASH) ||
          entry.key_equals(KEYWORD_VOCAB, KEYWORD_VOCAB_HASH) ||
          entry.key_equals(KEYWORD_LANGUAGE, KEYWORD_LANGUAGE_HASH) ||
          entry.key_equals(KEYWORD_VERSION, KEYWORD_VERSION_HASH) ||
          entry.key_equals(KEYWORD_DIRECTION, KEYWORD_DIRECTION_HASH) ||
          entry.key_equals(KEYWORD_IMPORT, KEYWORD_IMPORT_HASH) ||
          entry.key_equals(KEYWORD_PROPAGATE, KEYWORD_PROPAGATE_HASH) ||
          entry.key_equals(KEYWORD_PROTECTED, KEYWORD_PROTECTED_HASH)) {
        continue;
      }
      create_term_definition(state, active_context, context, name, defined,
                             location);
    }

    state.context_protected = saved_protected;
  }
}

} // namespace sourcemeta::core
