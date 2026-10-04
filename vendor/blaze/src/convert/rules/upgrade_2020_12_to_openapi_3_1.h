class Upgrade202012ToOpenAPI31 final : public SchemaTransformRule {
public:
  Upgrade202012ToOpenAPI31()
      : SchemaTransformRule{"upgrade_2020_12_to_openapi_3_1"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const Site &site, const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    // The OpenAPI dialect requires every 2020-12 vocabulary, so the core
    // vocabulary alone does not say the document is still below this rung.
    // Without the second half the rule would claim work on a document already
    // on the dialect and write the same `$schema` straight back
    ONLY_CONTINUE_IF(vocabularies.contains(
                         SchemaVocabularies::Known::JSON_SCHEMA_2020_12_CORE) &&
                     !vocabularies.contains_any(
                         {SchemaVocabularies::Known::OPENAPI_3_1_BASE,
                          SchemaVocabularies::Known::OPENAPI_3_2_BASE}));

    return at_dialect_declaration(schema, site);
  }

  auto transform(sourcemeta::core::JSON &schema, const Site &site) const
      -> void override {
    bump_dialect(schema, site, OPENAPI_3_1_DIALECT);
  }
};
