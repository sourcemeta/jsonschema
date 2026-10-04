class UpgradeOpenAPI31ToOpenAPI32 final : public SchemaTransformRule {
public:
  UpgradeOpenAPI31ToOpenAPI32()
      : SchemaTransformRule{"upgrade_openapi_3_1_to_openapi_3_2"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const Site &site, const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    // Asking for the 3.1 vocabulary rather than for 2020-12 is what keeps this
    // rule off a document that has yet to reach the rung below, which is the
    // other OpenAPI rule's business
    ONLY_CONTINUE_IF(
        vocabularies.contains(SchemaVocabularies::Known::OPENAPI_3_1_BASE) &&
        !vocabularies.contains(SchemaVocabularies::Known::OPENAPI_3_2_BASE));

    return at_dialect_declaration(schema, site);
  }

  auto transform(sourcemeta::core::JSON &schema, const Site &site) const
      -> void override {
    bump_dialect(schema, site, OPENAPI_3_2_DIALECT);
  }
};
