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

// Whether any subschema that names this one through `$schema` still has work
// of its own left. Such a referrer is read under the dialect this subschema
// defines, so moving this one first would take the referrer off the dialect
// the caller is acting on before its turn ever comes
template <typename Predicate>
auto has_pending_metaschema_referrer(
    const sourcemeta::core::JSON &root,
    const sourcemeta::core::SchemaFrame &frame,
    const sourcemeta::core::WeakPointer &pointer, const Predicate &pending)
    -> bool {
  return frame.any_reference(
      [&root, &frame, &pointer, &pending](
          const sourcemeta::core::SchemaReferenceType,
          const sourcemeta::core::WeakPointer &origin,
          const sourcemeta::core::SchemaFrame::Reference &reference) -> bool {
        if (origin.empty() || !origin.back().is_property() ||
            origin.back().to_property() != "$schema") {
          return false;
        }

        const auto destination{frame.traverse(reference.destination)};
        if (!destination.has_value() ||
            destination.value().get().pointer != pointer) {
          return false;
        }

        const auto referrer{sourcemeta::core::to_pointer(origin).initial()};
        const auto referrer_pointer{
            sourcemeta::core::to_weak_pointer(referrer)};

        // A meta-schema that describes itself is its own referrer, and waiting
        // on itself would leave it on the dialect it came in with for good
        if (referrer_pointer == pointer) {
          return false;
        }

        if (pending(sourcemeta::core::get(root, referrer))) {
          return true;
        }

        // Everything under the referrer is read under the dialect this
        // subschema defines too, so work down there counts just as much as
        // work on the referrer itself. Another meta-schema is governed by its
        // own referrers rather than by this one
        return frame.any_subschema_under(
            referrer_pointer,
            [&root, &frame, &pending](
                const sourcemeta::core::SchemaFrame::Location &entry) -> bool {
              const auto &entry_schema{sourcemeta::core::get(
                  root, sourcemeta::core::to_pointer(entry.pointer))};
              return !is_metaschema_target(entry_schema, frame,
                                           entry.pointer) &&
                     pending(entry_schema);
            });
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

inline auto moved_past(const sourcemeta::core::JSON &schema,
                       const std::string_view dialect) -> bool {
  const auto *override_value{schema.try_at(DIALECT_OVERRIDE_KEYWORD)};
  return override_value != nullptr && override_value->is_string() &&
         dialect_position(override_value->to_string()) >
             dialect_position(dialect);
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
  if (is_root || !moved_past(schema, dialect)) {
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
