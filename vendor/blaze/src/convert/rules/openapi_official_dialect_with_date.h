class OpenAPIOfficialDialectWithDate final : public SchemaTransformRule {
public:
  OpenAPIOfficialDialectWithDate()
      : SchemaTransformRule{"openapi_official_dialect_with_date"} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const sourcemeta::core::JSON &,
                               const sourcemeta::core::SchemaVocabularies &,
                               const Site &,
                               const sourcemeta::core::SchemaWalker &,
                               const sourcemeta::core::SchemaResolver &) const
      -> bool override {
    ONLY_CONTINUE_IF(schema.is_object());
    const auto *schema_keyword{schema.try_at("$schema")};
    ONLY_CONTINUE_IF(schema_keyword && schema_keyword->is_string());
    // The empty fragment does not change which dialect a URI names, and
    // `normalized_official_dialect` strips one before placing a dialect on the
    // ladder. Comparing the spelling as written would accept such a document as
    // this dialect and then decline to settle it onto the canonical URI
    const auto dialect{without_empty_fragment(schema_keyword->to_string())};
    ONLY_CONTINUE_IF(
        dialect == "https://spec.openapis.org/oas/3.1/dialect/2024-10-25" ||
        dialect == "https://spec.openapis.org/oas/3.1/dialect/2024-11-10");
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema, const Site &) const
      -> void override {
    schema.at("$schema").into(
        sourcemeta::core::JSON{std::string{OPENAPI_3_1_DIALECT}});
  }
};
