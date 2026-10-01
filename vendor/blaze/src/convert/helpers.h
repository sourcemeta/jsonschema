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

  // Core reads an empty override as no override at all, so shadowing a real
  // `$schema` with one would disagree with how the document frames
  const auto *override_value{schema.try_at(DIALECT_OVERRIDE_KEYWORD)};
  if (override_value != nullptr && override_value->is_string() &&
      !override_value->to_string().empty()) {
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

// How far along the ladder a dialect sits, counting from one so that anything
// the ladder does not name sits before all of them. The spelling is normalised
// first, so that every form naming the same dialect ranks the same. Whether a
// dialect is the ladder's and how far along it sits have to be one question,
// or the ladder would accept a marker in one place and refuse to rank it in
// another
inline auto dialect_position(const std::string_view dialect) -> std::size_t {
  const auto candidate{normalized_official_dialect(dialect)};
  for (std::size_t index = 0; index < LADDER_DIALECTS.size(); index += 1) {
    if (without_empty_fragment(LADDER_DIALECTS[index]) == candidate) {
      return index + 1;
    }
  }

  return 0;
}

// The official meta-schema documents, by the URI a schema references them at.
// Each recurses with the keyword of the dialect it was written for, so a
// meta-schema that extends one cannot be carried to another dialect by renaming
// anything in the extending document alone
// NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
constexpr std::array<std::string_view, 22> OFFICIAL_METASCHEMAS{
    {"http://json-schema.org/draft-03/schema",
     "http://json-schema.org/draft-04/schema",
     "http://json-schema.org/draft-06/schema",
     "http://json-schema.org/draft-07/schema",
     "https://json-schema.org/draft/2019-09/schema",
     "https://json-schema.org/draft/2019-09/meta/core",
     "https://json-schema.org/draft/2019-09/meta/applicator",
     "https://json-schema.org/draft/2019-09/meta/validation",
     "https://json-schema.org/draft/2019-09/meta/meta-data",
     "https://json-schema.org/draft/2019-09/meta/format",
     "https://json-schema.org/draft/2019-09/meta/content",
     "https://json-schema.org/draft/2019-09/meta/hyper-schema",
     "https://json-schema.org/draft/2020-12/schema",
     "https://json-schema.org/draft/2020-12/meta/core",
     "https://json-schema.org/draft/2020-12/meta/applicator",
     "https://json-schema.org/draft/2020-12/meta/unevaluated",
     "https://json-schema.org/draft/2020-12/meta/validation",
     "https://json-schema.org/draft/2020-12/meta/meta-data",
     "https://json-schema.org/draft/2020-12/meta/format-annotation",
     "https://json-schema.org/draft/2020-12/meta/format-assertion",
     "https://json-schema.org/draft/2020-12/meta/content",
     "https://json-schema.org/draft/2020-12/meta/hyper-schema"}};

// The two families settled on opposite schemes, `http` for the numbered drafts
// and `https` for the dated ones, and both spellings are seen in the wild.
// Which one a document wrote must not decide whether it is recognised
inline auto normalized_metaschema_uri(const std::string_view uri)
    -> std::string {
  std::string result{without_empty_fragment(uri)};
  if (result.starts_with("https://json-schema.org/draft-")) {
    result.erase(4, 1);
  } else if (result.starts_with("http://json-schema.org/draft/")) {
    result.insert(4, "s");
  }

  return result;
}

inline auto names_official_metaschema(const std::string_view uri) -> bool {
  const auto candidate{normalized_metaschema_uri(uri)};
  return std::ranges::any_of(
      OFFICIAL_METASCHEMAS, [&candidate](const auto &entry) -> bool {
        return normalized_metaschema_uri(entry) == candidate;
      });
}

// A dialect the ladder does not name is one the conversion has no rules for,
// whether it belongs to a draft older than the ladder starts at or to a
// meta-schema of the caller's own
inline auto names_ladder_dialect(const std::string_view dialect) -> bool {
  return dialect_position(dialect) > 0;
}

// Core reads this keyword as a dialect too, so it may well be a keyword the
// caller wrote. The ladder only ever records one of the dialects it walks
// through, so anything else is not ours to clear
inline auto is_own_dialect_override(const sourcemeta::core::JSON &value)
    -> bool {
  return value.is_string() && dialect_position(value.to_string()) > 0;
}

// Whether an identifier and a dialect name the same thing once both are
// resolved against what the caller said the document is called
inline auto names_the_same_uri(const sourcemeta::core::JSON &schema,
                               const sourcemeta::core::JSON::StringView keyword,
                               const std::string_view dialect,
                               const std::string_view default_id) -> bool {
  // A document that declares no usable identifier of its own is named by
  // whatever the caller said it is called, which is the answer identification
  // gives too, so that is the name this question has to ask about. Reading only
  // the keyword would let a meta-schema identified solely by the caller
  // describe itself unnoticed. A keyword holding something other than a string
  // never reaches here, as framing rejects it first
  const auto *identifier{schema.try_at(keyword)};
  const std::string_view candidate{
      identifier != nullptr && identifier->is_string()
          ? std::string_view{identifier->to_string()}
          : default_id};
  if (candidate.empty()) {
    return false;
  }

  if (without_empty_fragment(candidate) == without_empty_fragment(dialect)) {
    return true;
  }

  // Resolving is what lets an identifier written relative to whatever the
  // caller named the document meet a dialect that is spelled out in full.
  // A value that does not parse is not for this question to complain about,
  // as framing says so in better words a moment later
  try {
    sourcemeta::core::URI left{std::string{candidate}};
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

// A subschema that declares a dialect newer than the one a step moves schemas
// off has nothing pending for that step, as the keywords it holds belong to the
// dialect it declares rather than being strangers there. A subschema that
// declares nothing inherits the dialect of the schema being converted, which is
// why only a declared position counts
inline auto declares_newer_dialect(const sourcemeta::core::JSON &subschema,
                                   const std::string_view dialect) -> bool {
  const auto declared{dialect_position(declared_dialect(subschema))};
  return declared > 0 && declared > dialect_position(dialect);
}

// The ladder only ever writes its marker onto a schema, so clearing it follows
// the frame's own idea of where the schemas are rather than the spelling of
// keyword names. A keyword carries schemas only in a dialect that defines it,
// and walking a name the dialect never defined reaches the caller's own data,
// where a member that merely reads like the marker is not ours to touch. The
// frame is only in hand while a condition runs, so the rules carry this list
// over to their transform
inline auto subschema_pointers_under(
    const sourcemeta::core::SchemaFrame &frame,
    const sourcemeta::core::SchemaFrame::Location &location)
    -> std::vector<sourcemeta::core::Pointer> {
  std::vector<sourcemeta::core::Pointer> result;
  frame.for_each_subschema_under(
      location.pointer,
      [&result, &location](
          const sourcemeta::core::SchemaFrame::Location &entry) -> void {
        result.push_back(sourcemeta::core::to_pointer(entry.pointer)
                             .slice(location.pointer.size()));
      });

  return result;
}

// The marker is state of the ladder rather than of the schema. A resource that
// declares a dialect the conversion does not own can never materialise it into
// a `$schema`, so whatever survives the ladder has to come off before the
// caller ever sees it.
//
// Which members of a document are schemas is a question for the walker, so this
// follows the frame rather than guessing from keyword names. A keyword only
// carries schemas in a dialect that defines it, and walking a name the dialect
// never defined reaches the caller's own data, where a member that merely reads
// like the marker is not ours to touch
inline auto
erase_dialect_overrides(sourcemeta::core::JSON &schema,
                        const sourcemeta::core::SchemaWalker &walker,
                        const sourcemeta::core::SchemaResolver &resolver,
                        const std::string_view default_dialect,
                        const std::string_view default_id) -> void {
  if (!schema.is_object()) {
    return;
  }

  const sourcemeta::core::SchemaFrame frame{
      sourcemeta::core::SchemaFrame::Mode::References,
      schema,
      walker,
      resolver,
      default_dialect,
      default_id,
      sourcemeta::core::SchemaFrame::IdentifierMode::Fallback};

  std::vector<sourcemeta::core::Pointer> subschemas;
  frame.for_each_subschema(
      [&subschemas](
          const sourcemeta::core::SchemaFrame::Location &location) -> void {
        subschemas.push_back(sourcemeta::core::to_pointer(location.pointer));
      });

  for (const auto &pointer : subschemas) {
    auto &subschema{sourcemeta::core::get(schema, pointer)};
    if (!subschema.is_object()) {
      continue;
    }

    const auto *marker{subschema.try_at(DIALECT_OVERRIDE_KEYWORD)};
    if (marker != nullptr && is_own_dialect_override(*marker)) {
      subschema.erase(DIALECT_OVERRIDE_KEYWORD);
    }
  }
}

inline auto clear_dialect_override(sourcemeta::core::JSON &subschema,
                                   const bool is_root,
                                   const std::string_view dialect) -> void {
  if (!subschema.is_object()) {
    return;
  }

  if (!is_root && subschema.defines("$schema") &&
      subschema.at("$schema").is_string()) {
    return;
  }

  // A subschema that already moved past the dialect being established keeps
  // its marker. Dropping it would leave the keywords that move brought in
  // looking like keywords of the dialect it has left behind, and the rules
  // that reserve those names would prefix them away
  const auto *marker{subschema.try_at(DIALECT_OVERRIDE_KEYWORD)};
  if (marker != nullptr && is_own_dialect_override(*marker) &&
      (is_root || !moved_past(subschema, dialect))) {
    subschema.erase(DIALECT_OVERRIDE_KEYWORD);
  }
}

inline auto drop_dialect_overrides(
    sourcemeta::core::JSON &schema, const std::string_view dialect,
    const std::vector<sourcemeta::core::Pointer> &subschemas) -> void {
  clear_dialect_override(schema, true, dialect);

  for (const auto &pointer : subschemas) {
    if (pointer.empty()) {
      continue;
    }

    // A transform may have moved things before the markers come off, so a
    // location the frame knew about need not still be where it was
    if (sourcemeta::core::try_get(schema, pointer) == nullptr) {
      continue;
    }

    clear_dialect_override(sourcemeta::core::get(schema, pointer), false,
                           dialect);
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
