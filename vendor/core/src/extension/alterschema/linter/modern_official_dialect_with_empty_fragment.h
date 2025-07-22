class ModernOfficialDialectWithEmptyFragment final
    : public SchemaTransformRule {
public:
  ModernOfficialDialectWithEmptyFragment()
      : SchemaTransformRule{
            "modern_official_dialect_with_empty_fragment",
            "The official dialect URI of 2019-09 and newer versions must "
            "not contain the empty fragment"} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const sourcemeta::core::JSON &,
                               const sourcemeta::core::Vocabularies &,
                               const sourcemeta::core::SchemaFrame &,
                               const sourcemeta::core::SchemaFrame::Location &,
                               const sourcemeta::core::SchemaWalker &,
                               const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    if (!schema.is_object() || !schema.defines("$schema") ||
        !schema.at("$schema").is_string()) {
      return false;
    }

    const auto &schema_value = schema.at("$schema").to_string();
    return (
        schema_value == "https://json-schema.org/draft/2019-09/schema#" ||
        schema_value == "https://json-schema.org/draft/2019-09/hyper-schema#" ||
        schema_value == "https://json-schema.org/draft/2020-12/schema#" ||
        schema_value == "https://json-schema.org/draft/2020-12/hyper-schema#");
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    auto schema_value = schema.at("$schema").to_string();
    schema_value.pop_back();
    schema.at("$schema").into(sourcemeta::core::JSON{schema_value});
  }
};
