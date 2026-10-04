class UpgradeDraft6ToDraft7 final : public SchemaTransformRule {
public:
  UpgradeDraft6ToDraft7()
      : SchemaTransformRule{"upgrade_draft_6_to_draft_7"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const Site &site, const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains(SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6));

    return at_dialect_declaration(schema, site);
  }

  auto transform(sourcemeta::core::JSON &schema, const Site &site) const
      -> void override {
    bump_dialect(schema, site, DRAFT_7_URL);
  }

private:
  // NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
  static inline const std::string DRAFT_4_URL{
      "http://json-schema.org/draft-04/schema#"};
  // NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
  static inline const std::string DRAFT_6_URL{
      "http://json-schema.org/draft-06/schema#"};
  // NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
  static inline const std::string DRAFT_7_URL{
      "http://json-schema.org/draft-07/schema#"};
  // NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
  static inline const std::array<std::string_view, 8> PROMOTED_KEYWORDS{
      {"$comment", "if", "then", "else", "readOnly", "writeOnly",
       "contentMediaType", "contentEncoding"}};
};
