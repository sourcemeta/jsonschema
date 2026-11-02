#include "jsonld_algorithms.h"
#include "jsonld_keywords.h"

#include <sourcemeta/core/uri.h>

#include <algorithm> // std::ranges::sort
#include <cstddef>   // std::size_t
#include <optional>  // std::optional
#include <utility>   // std::move

namespace sourcemeta::core {

namespace {

// The location of one member of an array-valued @container entry, so that a
// combination spelled across several members names the one at fault
auto container_member_pointer(const WeakPointer &term_pointer,
                              const std::size_t index) -> Pointer {
  auto result{to_pointer(term_pointer)};
  result.push_back(JSON::String{KEYWORD_CONTAINER});
  result.push_back(index);
  return result;
}

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
    // Step 29.2: "Set definition to previous definition to retain the value of
    // protected", so a redefinition that says the same thing leaves what was
    // already there, origin included (JSON-LD 1.1 API Section 5.1.1 step 29.2)
    active_context.terms[term] = previous.value();
    defined[term] = true;
    return;
  }
  active_context.terms[term] = std::move(candidate);
  defined[term] = true;
}

// The mapping a term settles on must name a keyword, an IRI or a blank node,
// and may not stand in for the keyword that carries a context (JSON-LD 1.1 API
// Section 5.1.1 step 16.2.3)
auto check_iri_mapping(const ActiveContext &active_context,
                       const std::optional<JSON::String> &mapping,
                       const WeakPointer &term_pointer,
                       const std::initializer_list<JSON::StringView> children)
    -> void {
  if (!mapping.has_value() ||
      (!is_keyword(mapping.value()) && !mapping.value().contains(':') &&
       !active_context.vocabulary.has_value())) {
    throw JSONLDError("Invalid IRI mapping", term_pointer, children);
  }

  if (mapping.value() == KEYWORD_CONTEXT) {
    throw JSONLDError("Invalid keyword alias", term_pointer, children);
  }
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
    throw JSONLDError("Invalid term definition", context_pointer.concat(term));
  }

  defined[term] = false;
  const auto &value{local_context.at(term)};
  const WeakPointer term_pointer{context_pointer.concat(term)};

  if (is_keyword(term)) {
    if (term == KEYWORD_TYPE && value.is_object() && !state.processing_1_0) {
      TermDefinition type_definition;
      // Step 12 creates the definition "initializing prefix flag to false,
      // protected to protected", which context processing step 5.13 supplies
      // as "the value of the @protected entry from context, if any", and step
      // 13 then sets the flag "to the value of this entry", so an entry of the
      // definition overrides the context default in either direction (JSON-LD
      // 1.1 API Section 5.1.1 steps 12 and 13, Section 5.1 step 5.13)
      type_definition.is_protected = state.context_protected;
      bool has_container{false};
      bool has_protected{false};
      bool invalid_entry{false};
      for (const auto &entry : value.as_object()) {
        if (entry.key_equals(KEYWORD_PROTECTED, KEYWORD_PROTECTED_HASH)) {
          if (!entry.second.is_boolean()) {
            throw JSONLDError("Invalid @protected value", term_pointer,
                              {KEYWORD_PROTECTED});
          }
          type_definition.is_protected = entry.second.to_boolean();
          has_protected = true;
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
      // Step 4: the value of a @type definition "MUST be a map with only
      // either or both of the following entries: An entry for @container with
      // value @set. An entry for @protected", so either one alone is enough
      // and only a map carrying neither is an error. The shape is settled here
      // because step 4 precedes step 8, which reads the previous definition,
      // and step 29, which compares against it (JSON-LD 1.1 API Section 5.1.1
      // step 4)
      if (invalid_entry || (!has_container && !has_protected)) {
        throw JSONLDError("Keyword redefinition", term_pointer);
      }

      std::optional<TermDefinition> previous_type;
      const auto existing_type{active_context.terms.find(KEYWORD_TYPE)};
      if (existing_type != active_context.terms.cend()) {
        previous_type = existing_type->second;
      }

      finalize_definition(state, active_context, defined,
                          JSON::String{KEYWORD_TYPE}, term_pointer,
                          previous_type, std::move(type_definition));
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
  if (value.is_null()) {
    TermDefinition empty;
    empty.is_protected = state.context_protected;
    finalize_definition(state, active_context, defined, term, term_pointer,
                        previous, std::move(empty));
    return;
  }

  // A term whose identifier is explicitly null keeps no IRI mapping, which is
  // what retires it from expansion while leaving it to be redefined, and the
  // rest of what it says is still held to account (JSON-LD 1.1 API Section
  // 5.1.1 step 16.1)
  const bool explicit_null_id{id_entry != nullptr && id_entry->is_null()};

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
      } else if (term.contains('/')) {
        definition.iri = expand_iri(state, active_context, term, false, true,
                                    nullptr, nullptr, EMPTY_WEAK_POINTER);
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
        if (iri_like_colon || term.contains('/')) {
          auto probe{active_context};
          const auto expanded_term{expand_iri(state, probe, term, false, true,
                                              nullptr, nullptr,
                                              EMPTY_WEAK_POINTER)};
          if (expanded_term.has_value() && expanded_term != definition.iri) {
            throw JSONLDError("Invalid IRI mapping", term_pointer);
          }
        }
      }

      // A string definition stands for a map carrying that value under @id
      // (JSON-LD 1.1 API Section 5.1.1 step 10), so its mapping is held to the
      // same account as the spelled-out form
      check_iri_mapping(active_context, definition.iri, term_pointer, {});
    }
  } else if (value.is_object()) {
    const bool has_id{id_entry != nullptr};
    const JSON *const identifier{id_entry};

    // The protected flag and the type mapping are settled before the entry
    // that names the term is, so what either of them says wrong is reported
    // whichever way the term is named (JSON-LD 1.1 API Section 5.1.1 steps 13
    // and 14)
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
           !type.value().contains(':')) ||
          (state.processing_1_0 &&
           (type.value() == KEYWORD_JSON || type.value() == KEYWORD_NONE))) {
        throw JSONLDError("Invalid type mapping", term_pointer, {KEYWORD_TYPE});
      }
      definition.type_mapping = type;
    }

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
      // A reverse value that looks like a keyword is left alone, the keywords
      // themselves included (JSON-LD 1.1 API Section 5.1.1 step 15.3)
      if (has_keyword_form(reverse.to_string())) {
        defined[term] = true;
        return;
      }

      definition.reverse = true;
      definition.iri =
          expand_iri(state, active_context, reverse.to_string(), false, true,
                     &local_context, &defined, context_pointer);
      if (!definition.iri.has_value() ||
          !definition.iri.value().contains(':')) {
        throw JSONLDError("Invalid IRI mapping", term_pointer,
                          {KEYWORD_REVERSE});
      }
    } else if (has_id && !identifier->is_null() &&
               (!identifier->is_string() || identifier->to_string() != term)) {
      if (!identifier->is_string()) {
        throw JSONLDError("Invalid IRI mapping", term_pointer, {KEYWORD_ID});
      }
      const auto &id_value{identifier->to_string()};
      if (!is_keyword(id_value) && has_keyword_form(id_value)) {
        defined[term] = true;
        return;
      }
      definition.iri = expand_iri(state, active_context, id_value, false, true,
                                  &local_context, &defined, context_pointer);
      const auto &mapping{definition.iri};
      check_iri_mapping(active_context, mapping, term_pointer, {KEYWORD_ID});
      // In 1.1, a term that itself has the form of an IRI (a colon other than
      // at the edges, or a slash) must expand to its IRI mapping.
      if (!state.processing_1_0 && mapping.has_value()) {
        const auto colon_position{term.find(':')};
        const bool iri_like_colon{colon_position != JSON::String::npos &&
                                  colon_position != 0 &&
                                  colon_position + 1 != term.size()};
        if (iri_like_colon || term.contains('/')) {
          auto probe{active_context};
          const auto expanded_term{expand_iri(state, probe, term, false, true,
                                              nullptr, nullptr,
                                              EMPTY_WEAK_POINTER)};
          if (expanded_term.has_value() && expanded_term != mapping) {
            throw JSONLDError("Invalid IRI mapping", term_pointer,
                              {KEYWORD_ID});
          }
        }
      }
    } else if (explicit_null_id) {
      // No identifier is derived for a term that gave up its own
    } else if (term.contains(':') && !term.starts_with(':') &&
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
    } else if (term.contains('/')) {
      definition.iri = expand_iri(state, active_context, term, false, true,
                                  nullptr, nullptr, EMPTY_WEAK_POINTER);
    } else if (active_context.vocabulary.has_value()) {
      definition.iri = active_context.vocabulary.value() + term;
    }

    if (const auto *container_entry{
            value.try_at(KEYWORD_CONTAINER, KEYWORD_CONTAINER_HASH)}) {
      const auto &container{*container_entry};
      // A reverse property names one container, which must be @set or @index,
      // and an explicit null leaves no container mapping at all (JSON-LD 1.1
      // Section 9.15.1, JSON-LD 1.1 API Section 5.1.1 step 15.5)
      if (definition.reverse) {
        if (!container.is_null()) {
          if (!container.is_string() ||
              (container.to_string() != KEYWORD_SET &&
               container.to_string() != KEYWORD_INDEX)) {
            throw JSONLDError("Invalid reverse property", term_pointer,
                              {KEYWORD_CONTAINER});
          }

          definition.container.push_back(container.to_string());
        }
      } else if (container.is_array()) {
        // Array containers are a 1.1 feature.
        if (state.processing_1_0) {
          throw JSONLDError("Invalid container mapping", term_pointer,
                            {KEYWORD_CONTAINER});
        }
        std::size_t container_index{0};
        for (const auto &item : container.as_array()) {
          if (!item.is_string()) {
            throw JSONLDError(
                "Invalid container mapping",
                container_member_pointer(term_pointer, container_index));
          }
          const auto &item_string{item.to_string()};
          if (!is_valid_container(item_string)) {
            throw JSONLDError(
                "Invalid container mapping",
                container_member_pointer(term_pointer, container_index));
          }
          // A keyword may not appear more than once in the container array.
          for (const auto &seen : definition.container) {
            if (seen == item_string) {
              throw JSONLDError(
                  "Invalid container mapping",
                  container_member_pointer(term_pointer, container_index));
            }
          }
          definition.container.push_back(item_string);
          container_index += 1;
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

      // The combinations that may name several keywords accept them in any
      // order, so the same set spelled either way is the same mapping, and
      // storing it in one order is what lets a protected term be redefined
      // with the same mapping written differently
      std::ranges::sort(definition.container);

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
      // Valid array combinations (JSON-LD 1.1 API Section 5.1.1 step 21.1): a
      // single keyword, or @graph with exactly one of @id or @index optionally
      // with @set, or @set combined with any one of @index, @graph, @id,
      // @type, or @language.
      if (!definition.reverse && definition.container.size() != 1) {
        const bool graph_form{
            container_graph && (container_id != container_index) &&
            !container_list && !container_type && !container_language};
        const bool set_form{container_set && definition.container.size() == 2 &&
                            !container_list};
        if (!graph_form && !set_form) {
          throw JSONLDError("Invalid container mapping", term_pointer,
                            {KEYWORD_CONTAINER});
        }
      }
      // Step 21.4.1: "If type mapping in definition is undefined, set it to
      // @id", then step 21.4.2: "If type mapping in definition is neither @id
      // nor @vocab, an invalid type mapping error has been detected and
      // processing is aborted". The default is what coerces a non-map value of
      // the term to an identifier (JSON-LD 1.1 API Section 5.1.1 steps 21.4.1
      // and 21.4.2)
      if (container_type) {
        if (!definition.type_mapping.has_value()) {
          definition.type_mapping = JSON::String{KEYWORD_ID};
        } else if (definition.type_mapping.value() != KEYWORD_ID &&
                   definition.type_mapping.value() != KEYWORD_VOCAB) {
          throw JSONLDError("Invalid type mapping", term_pointer,
                            {KEYWORD_TYPE});
        }
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
      // The scoped context is processed here so that it is validated even
      // when the term is never used, and however it fails, the term is the one
      // at fault. A reference that is already loading is skipped rather than
      // followed (JSON-LD 1.1 API Section 5.1.1 step 21)
      const bool saved_override{state.protected_override};
      const bool saved_context_protected{state.context_protected};
      const bool saved_validate{state.validate_scoped_context};
      try {
        // The error raised here is always discarded below, so its location does
        // not matter.
        ActiveContext probe{active_context};
        state.protected_override = true;
        state.validate_scoped_context = false;
        process_context(state, probe, *context_entry, EMPTY_WEAK_POINTER);
        state.validate_scoped_context = saved_validate;
        state.protected_override = saved_override;
        state.context_protected = saved_context_protected;
      } catch (const JSONLDError &) {
        state.validate_scoped_context = saved_validate;
        state.protected_override = saved_override;
        state.context_protected = saved_context_protected;
        throw JSONLDError("Invalid scoped context", term_pointer,
                          {KEYWORD_CONTEXT});
      }
      definition.context = *context_entry;
      definition.context_base = state.context_resolution_base();
      // Step 21.7 keeps the local context of the definition to re-process when
      // the term comes into use, so whatever it says wrong then belongs where
      // it was written. Only a context the input spells out has entries of its
      // own to point into, which a context merged from an import settles one
      // term at a time (JSON-LD 1.1 API Section 5.1.1 step 21.7)
      definition.context_authored =
          !state.foreign_context_location.has_value() ||
          (state.input_context != nullptr &&
           state.input_context->defines(term));
      definition.context_location =
          definition.context_authored
              ? to_pointer(term_pointer.concat(keyword_context()))
              : state.foreign_context_location.value();
    }

    // The @reverse step of Create Term Definition sets the term definition and
    // returns before any @prefix processing, so a reverse property never takes
    // a prefix flag. (JSON-LD 1.1 API Section 4.2)
    if (const auto *prefix_entry{
            value.try_at(KEYWORD_PREFIX, KEYWORD_PREFIX_HASH)};
        prefix_entry != nullptr && !definition.reverse) {
      if (state.processing_1_0 || term.contains(':') || term.contains('/')) {
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
      // The expansion of the value has to name an IRI, which neither a
      // keyword nor a term that stayed relative does (JSON-LD 1.1 API Section
      // 5.1.1 step 21.2)
      if (!index_iri.has_value() || is_keyword(index_iri.value()) ||
          !URI::is_iri(index_iri.value())) {
        throw JSONLDError("Invalid term definition", term_pointer,
                          {KEYWORD_INDEX});
      }
      definition.index = index_string;
      definition.index_iri = index_iri.value();
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
        throw JSONLDError("Invalid term definition", term_pointer,
                          {entry.first});
      }
    }
  } else {
    throw JSONLDError("Invalid term definition", term_pointer);
  }

  if (simple_term && !term.contains(':') && !term.contains('/') &&
      definition.iri.has_value() &&
      (ends_with_gen_delim(definition.iri.value()) ||
       definition.iri.value().starts_with("_:"))) {
    definition.prefix = true;
  }

  if (!definition.reverse && !explicit_null_id && !definition.iri.has_value()) {
    throw JSONLDError("Invalid IRI mapping", term_pointer);
  }

  finalize_definition(state, active_context, defined, term, term_pointer,
                      previous, std::move(definition));
}

} // namespace sourcemeta::core
