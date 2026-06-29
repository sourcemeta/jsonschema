#include "jsonld_algorithms.h"
#include "jsonld_keywords.h"

#include <sourcemeta/core/uri.h>

#include <optional> // std::optional
#include <utility>  // std::move

namespace sourcemeta::core {

namespace {

// Whether the given value is a valid @container value.
auto is_valid_container(const JSON::StringView value) -> bool {
  return value == KEYWORD_LIST || value == KEYWORD_SET ||
         value == KEYWORD_INDEX || value == KEYWORD_LANGUAGE ||
         value == KEYWORD_ID || value == KEYWORD_TYPE || value == KEYWORD_GRAPH;
}

// Whether the given value ends with a URI generic delimiter (RFC 3986).
auto ends_with_gen_delim(const JSON::StringView value) -> bool {
  return !value.empty() && URI::is_gen_delim(value.back());
}

// Whether two definitions are equivalent ignoring their protected status, which
// is what a protected-term redefinition check compares.
auto same_definition(const TermDefinition &left, const TermDefinition &right)
    -> bool {
  return left.iri == right.iri && left.type_mapping == right.type_mapping &&
         left.container == right.container && left.language == right.language &&
         left.has_language == right.has_language &&
         left.direction == right.direction &&
         left.has_direction == right.has_direction &&
         left.context == right.context &&
         left.context_base == right.context_base && left.index == right.index &&
         left.index_iri == right.index_iri && left.nest == right.nest &&
         left.reverse == right.reverse && left.prefix == right.prefix;
}

// Store a freshly-built term definition, enforcing protected-term redefinition.
auto finalize_definition(ExpansionState &state, ActiveContext &active_context,
                         DefinedTerms &defined, const JSON::String &term,
                         const WeakPointer &term_pointer,
                         const std::optional<TermDefinition> &previous,
                         TermDefinition &&candidate) -> void {
  if (previous.has_value() && previous->is_protected &&
      !state.protected_override) {
    if (!same_definition(previous.value(), candidate)) {
      throw JSONLDError("Protected term redefinition", term_pointer);
    }
    // A redefinition with the same definition retains the protected flag.
    candidate.is_protected = true;
  }
  active_context.terms[term] = std::move(candidate);
  defined[term] = true;
}

} // namespace

// Create Term Definition (JSON-LD 1.1 API Section 5.1.1)
auto create_term_definition(ExpansionState &state,
                            ActiveContext &active_context,
                            const JSON &local_context, const JSON::String &term,
                            DefinedTerms &defined,
                            const WeakPointer &context_pointer) -> void {
  const auto status{defined.find(term)};
  if (status != defined.cend()) {
    if (status->second) {
      return;
    }
    throw JSONLDError("Cyclic IRI mapping", context_pointer.concat(term));
  }

  if (term.empty()) {
    throw JSONLDError("Invalid term definition", context_pointer);
  }

  defined[term] = false;
  const auto &value{local_context.at(term)};
  const WeakPointer term_pointer{context_pointer.concat(term)};

  if (is_keyword(term)) {
    if (term == KEYWORD_TYPE && value.is_object() && !state.processing_1_0) {
      TermDefinition type_definition;
      bool has_container{false};
      bool invalid_entry{false};
      for (const auto &entry : value.as_object()) {
        if (entry.key_equals(KEYWORD_PROTECTED, KEYWORD_PROTECTED_HASH)) {
          if (!entry.second.is_boolean()) {
            throw JSONLDError("Invalid @protected value", term_pointer,
                              {KEYWORD_PROTECTED});
          }
          type_definition.is_protected = entry.second.to_boolean();
        } else if (entry.key_equals(KEYWORD_CONTAINER,
                                    KEYWORD_CONTAINER_HASH) &&
                   entry.second.is_string()) {
          const auto &container{entry.second.to_string()};
          if (container == KEYWORD_SET) {
            type_definition.container.push_back(container);
            has_container = true;
          } else {
            invalid_entry = true;
          }
        } else {
          invalid_entry = true;
        }
      }
      // A redefinition of a protected @type is rejected before the shape of
      // the new definition is validated.
      const auto existing_type{active_context.terms.find(KEYWORD_TYPE)};
      if (existing_type != active_context.terms.cend() &&
          existing_type->second.is_protected && !state.protected_override) {
        if (!same_definition(existing_type->second, type_definition)) {
          throw JSONLDError("Protected term redefinition", term_pointer);
        }
        type_definition.is_protected = true;
      } else if (invalid_entry || !has_container) {
        throw JSONLDError("Keyword redefinition", term_pointer);
      } else if (!type_definition.is_protected) {
        type_definition.is_protected = state.context_protected;
      }
      active_context.terms[JSON::String{KEYWORD_TYPE}] =
          std::move(type_definition);
      defined[term] = true;
      return;
    }
    throw JSONLDError("Keyword redefinition", term_pointer);
  }

  if (has_keyword_form(term)) {
    defined[term] = true;
    return;
  }

  std::optional<TermDefinition> previous;
  const auto existing{active_context.terms.find(term)};
  if (existing != active_context.terms.cend()) {
    previous = existing->second;
  }
  active_context.terms.erase(term);

  const auto *id_entry{
      value.is_object() ? value.try_at(KEYWORD_ID, KEYWORD_ID_HASH) : nullptr};
  if (value.is_null() || (id_entry != nullptr && id_entry->is_null())) {
    TermDefinition empty;
    empty.is_protected = state.context_protected;
    // @protected is processed before the null @id is handled, so an explicitly
    // protected term that maps to null stays protected.
    if (id_entry != nullptr) {
      if (const auto *protected_entry{
              value.try_at(KEYWORD_PROTECTED, KEYWORD_PROTECTED_HASH)}) {
        if (!protected_entry->is_boolean()) {
          throw JSONLDError("Invalid @protected value", term_pointer,
                            {KEYWORD_PROTECTED});
        }
        if (state.processing_1_0) {
          throw JSONLDError("Invalid term definition", term_pointer,
                            {KEYWORD_PROTECTED});
        }
        empty.is_protected = protected_entry->to_boolean();
      }
    }
    finalize_definition(state, active_context, defined, term, term_pointer,
                        previous, std::move(empty));
    return;
  }

  TermDefinition definition;
  definition.is_protected = state.context_protected;
  bool simple_term{false};

  if (value.is_string()) {
    simple_term = true;
    const auto &string_value{value.to_string()};
    if (!is_keyword(string_value) && has_keyword_form(string_value)) {
      defined[term] = true;
      return;
    }
    if (string_value == term) {
      // A self-referential simple term resolves through the term itself.
      const auto colon{term.find(':')};
      if (colon != JSON::String::npos) {
        const auto prefix{term.substr(0, colon)};
        const auto suffix{term.substr(colon + 1)};
        if (prefix != "_" && !suffix.starts_with("//") &&
            local_context.is_object() && local_context.defines(prefix)) {
          const auto iterator{defined.find(prefix)};
          if (iterator == defined.cend() || !iterator->second) {
            create_term_definition(state, active_context, local_context, prefix,
                                   defined, context_pointer);
          }
        }
        const auto prefix_definition{active_context.terms.find(prefix)};
        if (prefix_definition != active_context.terms.cend() &&
            prefix_definition->second.iri.has_value()) {
          definition.iri = prefix_definition->second.iri.value() + suffix;
        } else {
          definition.iri = term;
        }
      } else if (term.find('/') != JSON::String::npos) {
        definition.iri = expand_iri(state, active_context, term, false, true,
                                    nullptr, nullptr, empty_weak_pointer);
      } else if (active_context.vocabulary.has_value()) {
        definition.iri = active_context.vocabulary.value() + term;
      }
    } else {
      definition.iri =
          expand_iri(state, active_context, string_value, false, true,
                     &local_context, &defined, context_pointer);
      // In 1.1, an IRI-like term must expand to its IRI mapping.
      if (!state.processing_1_0 && definition.iri.has_value()) {
        const auto colon_position{term.find(':')};
        const bool iri_like_colon{colon_position != JSON::String::npos &&
                                  colon_position != 0 &&
                                  colon_position + 1 != term.size()};
        if (iri_like_colon || term.find('/') != JSON::String::npos) {
          auto probe{active_context};
          const auto expanded_term{expand_iri(state, probe, term, false, true,
                                              nullptr, nullptr,
                                              empty_weak_pointer)};
          if (expanded_term.has_value() && expanded_term != definition.iri) {
            throw JSONLDError("Invalid IRI mapping", term_pointer);
          }
        }
      }
    }
  } else if (value.is_object()) {
    const bool has_id{id_entry != nullptr};
    const JSON *const id{id_entry};
    if (const auto *reverse_entry{
            value.try_at(KEYWORD_REVERSE, KEYWORD_REVERSE_HASH)}) {
      if (has_id || value.defines(KEYWORD_NEST, KEYWORD_NEST_HASH)) {
        throw JSONLDError("Invalid reverse property", term_pointer,
                          {KEYWORD_REVERSE});
      }
      const auto &reverse{*reverse_entry};
      if (!reverse.is_string()) {
        throw JSONLDError("Invalid IRI mapping", term_pointer,
                          {KEYWORD_REVERSE});
      }
      definition.reverse = true;
      definition.iri =
          expand_iri(state, active_context, reverse.to_string(), false, true,
                     &local_context, &defined, context_pointer);
      if (!definition.iri.has_value()) {
        // A reverse value with the form of a keyword is ignored.
        defined[term] = true;
        return;
      }
      if (definition.iri.value().find(':') == JSON::String::npos) {
        throw JSONLDError("Invalid IRI mapping", term_pointer,
                          {KEYWORD_REVERSE});
      }
    } else if (has_id && !id->is_null() &&
               (!id->is_string() || id->to_string() != term)) {
      if (!id->is_string()) {
        throw JSONLDError("Invalid IRI mapping", term_pointer, {KEYWORD_ID});
      }
      const auto &id_value{id->to_string()};
      if (!is_keyword(id_value) && has_keyword_form(id_value)) {
        defined[term] = true;
        return;
      }
      definition.iri = expand_iri(state, active_context, id_value, false, true,
                                  &local_context, &defined, context_pointer);
      const auto &mapping{definition.iri};
      if (!mapping.has_value() ||
          (!is_keyword(mapping.value()) &&
           mapping.value().find(':') == JSON::String::npos &&
           !active_context.vocabulary.has_value())) {
        throw JSONLDError("Invalid IRI mapping", term_pointer, {KEYWORD_ID});
      }
      if (mapping.has_value() && mapping.value() == KEYWORD_CONTEXT) {
        throw JSONLDError("Invalid keyword alias", term_pointer, {KEYWORD_ID});
      }
      // In 1.1, a term that itself has the form of an IRI (a colon other than
      // at the edges, or a slash) must expand to its IRI mapping.
      if (!state.processing_1_0 && mapping.has_value()) {
        const auto colon_position{term.find(':')};
        const bool iri_like_colon{colon_position != JSON::String::npos &&
                                  colon_position != 0 &&
                                  colon_position + 1 != term.size()};
        if (iri_like_colon || term.find('/') != JSON::String::npos) {
          auto probe{active_context};
          const auto expanded_term{expand_iri(state, probe, term, false, true,
                                              nullptr, nullptr,
                                              empty_weak_pointer)};
          if (expanded_term.has_value() && expanded_term != mapping) {
            throw JSONLDError("Invalid IRI mapping", term_pointer,
                              {KEYWORD_ID});
          }
        }
      }
    } else if (term.find(':') != JSON::String::npos && !term.starts_with(':') &&
               !term.ends_with(':')) {
      const auto colon{term.find(':')};
      const auto prefix{term.substr(0, colon)};
      const auto suffix{term.substr(colon + 1)};
      if (prefix != "_" && !suffix.starts_with("//") &&
          local_context.is_object() && local_context.defines(prefix)) {
        const auto iterator{defined.find(prefix)};
        if (iterator == defined.cend() || !iterator->second) {
          create_term_definition(state, active_context, local_context, prefix,
                                 defined, context_pointer);
        }
      }
      const auto prefix_definition{active_context.terms.find(prefix)};
      if (prefix_definition != active_context.terms.cend() &&
          prefix_definition->second.iri.has_value()) {
        definition.iri = prefix_definition->second.iri.value() + suffix;
      } else {
        definition.iri = term;
      }
    } else if (term.find('/') != JSON::String::npos) {
      definition.iri = expand_iri(state, active_context, term, false, true,
                                  nullptr, nullptr, empty_weak_pointer);
    } else if (active_context.vocabulary.has_value()) {
      definition.iri = active_context.vocabulary.value() + term;
    }

    if (const auto *type_entry{value.try_at(KEYWORD_TYPE, KEYWORD_TYPE_HASH)}) {
      const auto &type_value{*type_entry};
      if (!type_value.is_string()) {
        throw JSONLDError("Invalid type mapping", term_pointer, {KEYWORD_TYPE});
      }
      const auto type{expand_iri(state, active_context, type_value.to_string(),
                                 false, true, &local_context, &defined,
                                 context_pointer)};
      if (!type.has_value() || type.value().starts_with("_:") ||
          (type.value() != KEYWORD_ID && type.value() != KEYWORD_VOCAB &&
           type.value() != KEYWORD_JSON && type.value() != KEYWORD_NONE &&
           type.value().find(':') == JSON::String::npos) ||
          (state.processing_1_0 &&
           (type.value() == KEYWORD_JSON || type.value() == KEYWORD_NONE))) {
        throw JSONLDError("Invalid type mapping", term_pointer, {KEYWORD_TYPE});
      }
      definition.type_mapping = type;
    }

    if (const auto *container_entry{
            value.try_at(KEYWORD_CONTAINER, KEYWORD_CONTAINER_HASH)}) {
      const auto &container{*container_entry};
      if (container.is_array()) {
        // Array containers are a 1.1 feature.
        if (state.processing_1_0) {
          throw JSONLDError("Invalid container mapping", term_pointer,
                            {KEYWORD_CONTAINER});
        }
        for (const auto &item : container.as_array()) {
          if (!item.is_string()) {
            throw JSONLDError("Invalid container mapping", term_pointer,
                              {KEYWORD_CONTAINER});
          }
          const auto &item_string{item.to_string()};
          if (!is_valid_container(item_string)) {
            throw JSONLDError("Invalid container mapping", term_pointer,
                              {KEYWORD_CONTAINER});
          }
          // A keyword may not appear more than once in the container array.
          for (const auto &seen : definition.container) {
            if (seen == item_string) {
              throw JSONLDError("Invalid container mapping", term_pointer,
                                {KEYWORD_CONTAINER});
            }
          }
          definition.container.push_back(item_string);
        }
      } else if (container.is_string()) {
        const auto &container_string{container.to_string()};
        // In 1.0, the @graph, @id and @type containers are not permitted.
        if (state.processing_1_0 && (container_string == KEYWORD_GRAPH ||
                                     container_string == KEYWORD_ID ||
                                     container_string == KEYWORD_TYPE)) {
          throw JSONLDError("Invalid container mapping", term_pointer,
                            {KEYWORD_CONTAINER});
        }
        if (!is_valid_container(container_string)) {
          throw JSONLDError("Invalid container mapping", term_pointer,
                            {KEYWORD_CONTAINER});
        }
        definition.container.push_back(container_string);
      } else {
        throw JSONLDError("Invalid container mapping", term_pointer,
                          {KEYWORD_CONTAINER});
      }
      if (definition.reverse) {
        for (const auto &item : definition.container) {
          if (item != KEYWORD_SET && item != KEYWORD_INDEX) {
            throw JSONLDError("Invalid reverse property", term_pointer,
                              {KEYWORD_CONTAINER});
          }
        }
      }
      bool container_graph{false};
      bool container_id{false};
      bool container_index{false};
      bool container_language{false};
      bool container_list{false};
      bool container_set{false};
      bool container_type{false};
      for (const auto &item : definition.container) {
        if (item == KEYWORD_GRAPH) {
          container_graph = true;
        } else if (item == KEYWORD_ID) {
          container_id = true;
        } else if (item == KEYWORD_INDEX) {
          container_index = true;
        } else if (item == KEYWORD_LANGUAGE) {
          container_language = true;
        } else if (item == KEYWORD_LIST) {
          container_list = true;
        } else if (item == KEYWORD_SET) {
          container_set = true;
        } else if (item == KEYWORD_TYPE) {
          container_type = true;
        }
      }
      // Valid array combinations (JSON-LD 1.1 API Section 5.1.1 step 19.1): a
      // single keyword, or @graph with exactly one of @id or @index optionally
      // with @set, or @set combined with any of @index, @graph, @id, @type, or
      // @language.
      if (definition.container.size() != 1) {
        const bool graph_form{
            container_graph && (container_id != container_index) &&
            !container_list && !container_type && !container_language};
        const bool set_form{container_set && !container_list};
        if (!graph_form && !set_form) {
          throw JSONLDError("Invalid container mapping", term_pointer,
                            {KEYWORD_CONTAINER});
        }
      }
      // A type-map container may only coerce its keys to identifiers.
      if (container_type && definition.type_mapping.has_value() &&
          definition.type_mapping.value() != KEYWORD_ID &&
          definition.type_mapping.value() != KEYWORD_VOCAB) {
        throw JSONLDError("Invalid type mapping", term_pointer, {KEYWORD_TYPE});
      }
    }

    if (const auto *language_entry{
            value.try_at(KEYWORD_LANGUAGE, KEYWORD_LANGUAGE_HASH)};
        language_entry != nullptr &&
        !value.defines(KEYWORD_TYPE, KEYWORD_TYPE_HASH)) {
      const auto &language{*language_entry};
      if (!language.is_null() && !language.is_string()) {
        throw JSONLDError("Invalid language mapping", term_pointer,
                          {KEYWORD_LANGUAGE});
      }
      definition.has_language = true;
      if (language.is_string()) {
        definition.language = language.to_string();
      }
    }

    if (const auto *direction_entry{
            value.try_at(KEYWORD_DIRECTION, KEYWORD_DIRECTION_HASH)};
        direction_entry != nullptr &&
        !value.defines(KEYWORD_TYPE, KEYWORD_TYPE_HASH)) {
      const auto &direction{*direction_entry};
      if (!direction.is_null() &&
          (!direction.is_string() || (direction.to_string() != "ltr" &&
                                      direction.to_string() != "rtl"))) {
        throw JSONLDError("Invalid base direction", term_pointer,
                          {KEYWORD_DIRECTION});
      }
      definition.has_direction = true;
      if (direction.is_string()) {
        definition.direction = direction.to_string();
      }
    }

    if (const auto *context_entry{
            value.try_at(KEYWORD_CONTEXT, KEYWORD_CONTEXT_HASH)}) {
      if (state.processing_1_0) {
        throw JSONLDError("Invalid term definition", term_pointer,
                          {KEYWORD_CONTEXT});
      }
      // Validate the scoped context eagerly so that errors surface even when
      // the term is never used. Remote scoped contexts (including recursive
      // ones) are validated lazily when the term is used instead.
      const bool saved_override{state.protected_override};
      const bool saved_context_protected{state.context_protected};
      try {
        // The error raised here is always discarded below, so its location does
        // not matter.
        ActiveContext probe{active_context};
        state.protected_override = true;
        process_context(state, probe, *context_entry, empty_weak_pointer);
        state.protected_override = saved_override;
        state.context_protected = saved_context_protected;
      } catch (const JSONLDError &error) {
        state.protected_override = saved_override;
        state.context_protected = saved_context_protected;
        const JSON::StringView code{error.what()};
        if (code != "Loading remote context failed" &&
            code != "Recursive context inclusion" &&
            code != "Invalid remote context") {
          throw JSONLDError("Invalid scoped context", term_pointer,
                            {KEYWORD_CONTEXT});
        }
      }
      definition.context = *context_entry;
      definition.context_base = state.context_resolution_base();
    }

    if (const auto *prefix_entry{
            value.try_at(KEYWORD_PREFIX, KEYWORD_PREFIX_HASH)}) {
      if (state.processing_1_0 || term.find(':') != JSON::String::npos ||
          term.find('/') != JSON::String::npos) {
        throw JSONLDError("Invalid term definition", term_pointer,
                          {KEYWORD_PREFIX});
      }
      if (!prefix_entry->is_boolean()) {
        throw JSONLDError("Invalid @prefix value", term_pointer,
                          {KEYWORD_PREFIX});
      }
      definition.prefix = prefix_entry->to_boolean();
      if (definition.prefix && definition.iri.has_value() &&
          is_keyword(definition.iri.value())) {
        throw JSONLDError("Invalid term definition", term_pointer,
                          {KEYWORD_PREFIX});
      }
    }

    if (const auto *nest_entry{value.try_at(KEYWORD_NEST, KEYWORD_NEST_HASH)}) {
      if (state.processing_1_0) {
        throw JSONLDError("Invalid term definition", term_pointer,
                          {KEYWORD_NEST});
      }
      const auto &nest{*nest_entry};
      if (!nest.is_string()) {
        throw JSONLDError("Invalid @nest value", term_pointer, {KEYWORD_NEST});
      }
      const auto &nest_string{nest.to_string()};
      if (is_keyword(nest_string) && nest_string != KEYWORD_NEST) {
        throw JSONLDError("Invalid @nest value", term_pointer, {KEYWORD_NEST});
      }
      definition.nest = nest_string;
    }

    if (const auto *index_entry{
            value.try_at(KEYWORD_INDEX, KEYWORD_INDEX_HASH)}) {
      if (state.processing_1_0) {
        throw JSONLDError("Invalid term definition", term_pointer,
                          {KEYWORD_INDEX});
      }
      bool has_index_container{false};
      for (const auto &item : definition.container) {
        if (item == KEYWORD_INDEX) {
          has_index_container = true;
        }
      }
      const auto &index{*index_entry};
      if (!index.is_string() || !has_index_container) {
        throw JSONLDError("Invalid term definition", term_pointer,
                          {KEYWORD_INDEX});
      }
      const auto &index_string{index.to_string()};
      const auto index_iri{expand_iri(state, active_context, index_string,
                                      false, true, &local_context, &defined,
                                      context_pointer)};
      if (!index_iri.has_value() || is_keyword(index_iri.value())) {
        throw JSONLDError("Invalid term definition", term_pointer,
                          {KEYWORD_INDEX});
      }
      definition.index = index_string;
      definition.index_iri = index_iri.value();
    }

    if (const auto *protected_entry{
            value.try_at(KEYWORD_PROTECTED, KEYWORD_PROTECTED_HASH)}) {
      if (!protected_entry->is_boolean()) {
        throw JSONLDError("Invalid @protected value", term_pointer,
                          {KEYWORD_PROTECTED});
      }
      if (state.processing_1_0) {
        throw JSONLDError("Invalid term definition", term_pointer,
                          {KEYWORD_PROTECTED});
      }
      definition.is_protected = protected_entry->to_boolean();
    }

    // A term definition may not contain any entry other than the keywords
    // recognised above.
    for (const auto &entry : value.as_object()) {
      if (!entry.key_equals(KEYWORD_ID, KEYWORD_ID_HASH) &&
          !entry.key_equals(KEYWORD_REVERSE, KEYWORD_REVERSE_HASH) &&
          !entry.key_equals(KEYWORD_CONTAINER, KEYWORD_CONTAINER_HASH) &&
          !entry.key_equals(KEYWORD_CONTEXT, KEYWORD_CONTEXT_HASH) &&
          !entry.key_equals(KEYWORD_DIRECTION, KEYWORD_DIRECTION_HASH) &&
          !entry.key_equals(KEYWORD_INDEX, KEYWORD_INDEX_HASH) &&
          !entry.key_equals(KEYWORD_LANGUAGE, KEYWORD_LANGUAGE_HASH) &&
          !entry.key_equals(KEYWORD_NEST, KEYWORD_NEST_HASH) &&
          !entry.key_equals(KEYWORD_PREFIX, KEYWORD_PREFIX_HASH) &&
          !entry.key_equals(KEYWORD_PROTECTED, KEYWORD_PROTECTED_HASH) &&
          !entry.key_equals(KEYWORD_TYPE, KEYWORD_TYPE_HASH)) {
        throw JSONLDError("Invalid term definition", term_pointer);
      }
    }
  } else {
    throw JSONLDError("Invalid term definition", term_pointer);
  }

  if (simple_term && term.find(':') == JSON::String::npos &&
      term.find('/') == JSON::String::npos && definition.iri.has_value() &&
      (ends_with_gen_delim(definition.iri.value()) ||
       definition.iri.value().starts_with("_:"))) {
    definition.prefix = true;
  }

  if (!definition.reverse && !definition.iri.has_value()) {
    throw JSONLDError("Invalid IRI mapping", term_pointer);
  }

  finalize_definition(state, active_context, defined, term, term_pointer,
                      previous, std::move(definition));
}

} // namespace sourcemeta::core
