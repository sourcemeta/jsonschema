#ifndef SOURCEMETA_BLAZE_CONVERT_HELPERS_H_
#define SOURCEMETA_BLAZE_CONVERT_HELPERS_H_

// NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
const std::string DIALECT_OVERRIDE_KEYWORD{
    "x-sourcemeta-dialect-override-subschema"};

// The dialect a schema declares, honouring the marker that the upgrade rules
// leave behind while they walk a document across drafts
inline auto declared_dialect(const sourcemeta::core::JSON &schema)
    -> std::string_view {
  if (!schema.is_object()) {
    return {};
  }

  const auto *override_value{schema.try_at(DIALECT_OVERRIDE_KEYWORD)};
  if (override_value != nullptr && override_value->is_string()) {
    return override_value->to_string();
  }

  const auto *dialect{schema.try_at("$schema")};
  if (dialect != nullptr && dialect->is_string()) {
    return dialect->to_string();
  }

  return {};
}

inline auto mark_dialect_override(sourcemeta::core::JSON &schema,
                                  const std::string_view dialect) -> void {
  schema.assign(DIALECT_OVERRIDE_KEYWORD, sourcemeta::core::JSON{dialect});
}

inline auto current_dialect_or_override(const sourcemeta::core::JSON &schema)
    -> std::string_view {
  return declared_dialect(schema);
}

// The empty fragment does not change which dialect a URI names, and only some
// of the official spellings have a rule of their own to settle them
inline auto without_empty_fragment(const std::string_view uri)
    -> std::string_view {
  return uri.ends_with('#') ? uri.substr(0, uri.size() - 1) : uri;
}

// A subschema that a `$schema` of the document resolves to is a meta-schema of
// that document, no matter where within the document it sits. Every base
// dialect asks such a subschema to declare an identifier, which is what keeps
// the scan off the subschemas that could never be named that way
inline auto is_metaschema_target(const sourcemeta::core::JSON &schema,
                                 const sourcemeta::core::SchemaFrame &frame,
                                 const sourcemeta::core::WeakPointer &pointer)
    -> bool {
  if (!schema.is_object() || !schema.defines_any({"$id", "id"})) {
    return false;
  }

  // A document that takes its dialect from the caller rather than from a
  // `$schema` of its own names no meta-schema anywhere, so what the document
  // reads as is the only thing left to ask
  const auto document{frame.traverse(sourcemeta::core::EMPTY_WEAK_POINTER)};
  if (document.has_value()) {
    const auto target{frame.traverse(document.value().get().dialect)};
    if (target.has_value() && target.value().get().pointer == pointer) {
      return true;
    }
  }

  return frame.any_reference(
      [&frame, &pointer](
          const sourcemeta::core::SchemaReferenceType,
          const sourcemeta::core::WeakPointer &origin,
          const sourcemeta::core::SchemaFrame::Reference &reference) -> bool {
        if (origin.empty() || !origin.back().is_property() ||
            origin.back().to_property() != "$schema") {
          return false;
        }

        const auto destination{frame.traverse(reference.destination)};
        return destination.has_value() &&
               destination.value().get().pointer == pointer;
      });
}

inline auto
subschema_at_dialect(const sourcemeta::core::JSON &schema,
                     const sourcemeta::core::SchemaFrame::Location &location,
                     const std::string_view dialect) -> bool {
  const auto current{current_dialect_or_override(schema)};
  if (!current.empty()) {
    return current == dialect;
  }
  return schema.is_object() && location.pointer.empty();
}

// The official dialects the upgrade walks through, oldest first, so that a
// marker recording a newer one can be told apart from a stale one
// NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
constexpr std::array<std::string_view, 6> LADDER_DIALECTS{
    {"http://json-schema.org/draft-03/schema#",
     "http://json-schema.org/draft-04/schema#",
     "http://json-schema.org/draft-06/schema#",
     "http://json-schema.org/draft-07/schema#",
     "https://json-schema.org/draft/2019-09/schema",
     "https://json-schema.org/draft/2020-12/schema"}};

// How far along the ladder a dialect sits, counting from one so that anything
// the ladder does not name sits before all of them
inline auto dialect_position(const std::string_view dialect) -> std::size_t {
  for (std::size_t index = 0; index < LADDER_DIALECTS.size(); index += 1) {
    if (LADDER_DIALECTS[index] == dialect) {
      return index + 1;
    }
  }

  return 0;
}

// Core reads this keyword as a dialect too, so it may well be a keyword the
// caller wrote. The ladder only ever records one of the dialects it walks
// through, so anything else is not ours to clear
inline auto is_own_dialect_override(const sourcemeta::core::JSON &value)
    -> bool {
  return value.is_string() && dialect_position(value.to_string()) > 0;
}

// The spellings the normalising rules settle on all name the same dialect, so
// whether the ladder names one has to be asked of the spelling those rules
// would produce rather than of what the document happens to say
inline auto normalized_official_dialect(const std::string_view dialect)
    -> std::string {
  std::string result{without_empty_fragment(dialect)};
  if (result.starts_with("https://json-schema.org/draft-")) {
    result.erase(4, 1);
  }

  return result;
}

// A dialect the ladder does not name is one the conversion has no rules for,
// whether it belongs to a draft older than the ladder starts at or to a
// meta-schema of the caller's own
inline auto names_ladder_dialect(const std::string_view dialect) -> bool {
  const auto candidate{normalized_official_dialect(dialect)};
  return std::ranges::any_of(
      LADDER_DIALECTS, [&candidate](const auto &entry) -> bool {
        return without_empty_fragment(entry) == candidate;
      });
}

// Whether an identifier and a dialect name the same thing once both are
// resolved against what the caller said the document is called
inline auto names_the_same_uri(const sourcemeta::core::JSON &schema,
                               const sourcemeta::core::JSON::StringView keyword,
                               const std::string_view dialect,
                               const std::string_view default_id) -> bool {
  const auto *identifier{schema.try_at(keyword)};
  if (identifier == nullptr || !identifier->is_string()) {
    return false;
  }

  if (without_empty_fragment(identifier->to_string()) ==
      without_empty_fragment(dialect)) {
    return true;
  }

  // Resolving is what lets an identifier written relative to whatever the
  // caller named the document meet a dialect that is spelled out in full.
  // A value that does not parse is not for this question to complain about,
  // as framing says so in better words a moment later
  try {
    sourcemeta::core::URI left{identifier->to_string()};
    sourcemeta::core::URI right{std::string{dialect}};
    if (!default_id.empty()) {
      const sourcemeta::core::URI base{std::string{default_id}};
      left.resolve_from(base);
      right.resolve_from(base);
    }

    left.canonicalize();
    right.canonicalize();
    return left.recompose() == right.recompose();
  } catch (const sourcemeta::core::URIParseError &) {
    return false;
  } catch (const sourcemeta::core::URIError &) {
    return false;
  }
}

// A document whose identifier is the very dialect it declares describes
// itself, so it is a meta-schema on the strongest evidence there is. The
// ladder rewrites that `$schema` on the first bump, taking the evidence with
// it, so the question has to be asked before any rule runs
inline auto
describes_itself(const sourcemeta::core::JSON &schema,
                 const sourcemeta::core::SchemaBaseDialect base_dialect,
                 const std::string_view default_id) -> bool {
  if (!schema.is_object()) {
    return false;
  }

  const auto *dialect{schema.try_at("$schema")};
  if (dialect == nullptr || !dialect->is_string()) {
    return false;
  }

  // Draft 3 and Draft 4 carry the identifier in `id` and everything after them
  // in `$id`, so the other keyword is ordinary data there. Which one is which
  // is the base dialect's answer to give, not something to read off a URI that
  // has more than one accepted spelling
  return names_the_same_uri(
      schema, sourcemeta::core::schema_identifier_keyword(base_dialect),
      dialect->to_string(), default_id);
}

inline auto moved_past(const sourcemeta::core::JSON &schema,
                       const std::string_view dialect) -> bool {
  const auto *override_value{schema.try_at(DIALECT_OVERRIDE_KEYWORD)};
  return override_value != nullptr && override_value->is_string() &&
         dialect_position(override_value->to_string()) >
             dialect_position(dialect);
}

// The marker is state of the ladder rather than of the schema. A resource that
// declares a dialect the conversion does not own can never materialise it into
// a `$schema`, so whatever survives the ladder has to come off before the
// caller ever sees it
inline auto erase_dialect_overrides(sourcemeta::core::JSON &schema) -> void {
  if (schema.is_array()) {
    for (auto &item : schema.as_array()) {
      erase_dialect_overrides(item);
    }

    return;
  }

  if (!schema.is_object()) {
    return;
  }

  const auto *marker{schema.try_at(DIALECT_OVERRIDE_KEYWORD)};
  if (marker != nullptr && is_own_dialect_override(*marker)) {
    schema.erase(DIALECT_OVERRIDE_KEYWORD);
  }

  std::vector<std::string> keys;
  keys.reserve(schema.size());
  for (const auto &entry : schema.as_object()) {
    keys.push_back(entry.first);
  }
  for (const auto &key : keys) {
    erase_dialect_overrides(schema.at(key));
  }
}

inline auto drop_dialect_overrides(sourcemeta::core::JSON &schema,
                                   const bool is_root,
                                   const std::string_view dialect) -> void {
  if (schema.is_array()) {
    for (auto &item : schema.as_array()) {
      drop_dialect_overrides(item, false, dialect);
    }
    return;
  }

  if (!schema.is_object()) {
    return;
  }

  if (!is_root && schema.defines("$schema") &&
      schema.at("$schema").is_string()) {
    return;
  }

  // A subschema that already moved past the dialect being established keeps
  // its marker. Dropping it would leave the keywords that move brought in
  // looking like keywords of the dialect it has left behind, and the rules
  // that reserve those names would prefix them away
  const auto *marker{schema.try_at(DIALECT_OVERRIDE_KEYWORD)};
  if (marker != nullptr && is_own_dialect_override(*marker) &&
      (is_root || !moved_past(schema, dialect))) {
    schema.erase(DIALECT_OVERRIDE_KEYWORD);
  }

  std::vector<std::string> keys;
  keys.reserve(schema.size());
  for (const auto &entry : schema.as_object()) {
    keys.push_back(entry.first);
  }
  for (const auto &key : keys) {
    drop_dialect_overrides(schema.at(key), false, dialect);
  }
}

struct AnchorCharPolicy {
  std::function<bool(char)> is_valid_first;
  std::function<bool(char)> is_valid_body;
};

inline auto sanitize_anchor_with_policy(const std::string_view original,
                                        const std::set<std::string> &in_use,
                                        const AnchorCharPolicy &policy)
    -> std::string {
  std::string sanitized;
  sanitized.reserve(original.size());
  for (const char character : original) {
    sanitized.push_back(policy.is_valid_body(character) ? character : '-');
  }
  while (sanitized.empty() || !policy.is_valid_first(sanitized.front()) ||
         in_use.contains(sanitized)) {
    sanitized.insert(0, "x-");
  }
  return sanitized;
}

#define ONLY_CONTINUE_IF(condition)                                            \
  if (!(condition)) {                                                          \
    return false;                                                              \
  }

#endif
