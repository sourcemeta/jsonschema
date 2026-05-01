class UpgradeDraft6ToDraft7 final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  UpgradeDraft6ToDraft7()
      : SchemaTransformRule{"upgrade_draft_6_to_draft_7", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &root,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains(Vocabularies::Known::JSON_Schema_Draft_6) &&
        schema.is_object() && schema.defines("$schema") &&
        schema.at("$schema").is_string() &&
        schema.at("$schema").to_string() == DRAFT_6_URL);

    for (const auto &entry : frame.locations()) {
      if (entry.second.type !=
              sourcemeta::core::SchemaFrame::LocationType::Resource &&
          entry.second.type !=
              sourcemeta::core::SchemaFrame::LocationType::Subschema) {
        continue;
      }

      if (!is_strict_descendant(location.pointer, entry.second.pointer)) {
        continue;
      }

      const auto entry_pointer{
          sourcemeta::core::to_pointer(entry.second.pointer)};
      const auto &entry_schema{sourcemeta::core::get(root, entry_pointer)};

      if (entry_schema.is_object() && entry_schema.defines("$ref")) {
        continue;
      }

      if (has_pending_pattern(entry_schema)) {
        return false;
      }
    }

    return true;
  }

  auto transform(sourcemeta::core::JSON &schema, const Result &) const
      -> void override {
    schema.assign("$schema", sourcemeta::core::JSON{DRAFT_7_URL});
    drop_dialect_overrides(schema, true);
  }

private:
  static inline const std::string DRAFT_4_URL{
      "http://json-schema.org/draft-04/schema#"};
  static inline const std::string DRAFT_6_URL{
      "http://json-schema.org/draft-06/schema#"};
  static inline const std::string DRAFT_7_URL{
      "http://json-schema.org/draft-07/schema#"};
  static inline const std::array<std::string_view, 8> PROMOTED_KEYWORDS{
      {"$comment", "if", "then", "else", "readOnly", "writeOnly",
       "contentMediaType", "contentEncoding"}};

  static auto has_pending_pattern(const sourcemeta::core::JSON &subschema)
      -> bool {
    if (!subschema.is_object()) {
      return false;
    }

    if (subschema.defines("$schema") && subschema.at("$schema").is_string()) {
      const auto &dialect{subschema.at("$schema").to_string()};
      if (dialect == DRAFT_4_URL || dialect == DRAFT_6_URL) {
        return true;
      }
    }

    for (const auto &keyword : PROMOTED_KEYWORDS) {
      if (subschema.defines(std::string{keyword})) {
        return true;
      }
    }

    return false;
  }

  static auto
  is_strict_descendant(const sourcemeta::core::WeakPointer &ancestor,
                       const sourcemeta::core::WeakPointer &candidate) -> bool {
    if (candidate.size() <= ancestor.size()) {
      return false;
    }
    for (std::size_t index{0}; index < ancestor.size(); ++index) {
      if (!(ancestor.at(index) == candidate.at(index))) {
        return false;
      }
    }
    return true;
  }
};
