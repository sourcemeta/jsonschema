class OpenAPIOfficialDialectWithEmptyFragment final
    : public SchemaTransformRule {
public:
  OpenAPIOfficialDialectWithEmptyFragment()
      : SchemaTransformRule{"openapi_official_dialect_with_empty_fragment"} {};

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
    const auto &dialect{schema_keyword->to_string()};
    // Only the spellings this rule owns. The dated 3.1 URIs are absent on
    // purpose: `OpenAPIOfficialDialectWithDate` is registered ahead of this
    // rule and strips the empty fragment before comparing, so it settles one
    // of those onto the undated URI whether or not a fragment was written,
    // and this rule's condition is asked again right before its transform.
    // Listing them here would be a branch nothing can reach
    ONLY_CONTINUE_IF(
        dialect == "https://spec.openapis.org/oas/3.1/dialect/base#" ||
        dialect == "https://spec.openapis.org/oas/3.2/dialect/2025-09-17#");
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema, const Site &) const
      -> void override {
    auto dialect{std::move(schema.at("$schema")).to_string()};
    dialect.pop_back();
    schema.at("$schema").into(sourcemeta::core::JSON{dialect});
  }
};
