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
    ONLY_CONTINUE_IF(schema.is_object() && schema.defines("$schema") &&
                     schema.at("$schema").is_string());
    const auto &dialect{schema.at("$schema").to_string()};
    ONLY_CONTINUE_IF(
        dialect == "https://json-schema.org/draft/2019-09/schema#" ||
        dialect == "https://json-schema.org/draft/2019-09/hyper-schema#" ||
        dialect == "https://json-schema.org/draft/2020-12/schema#" ||
        dialect == "https://json-schema.org/draft/2020-12/hyper-schema#");
    return APPLIES_TO_KEYWORDS("$schema");
  }

  auto transform(sourcemeta::core::JSON &schema, const Result &) const
      -> void override {
    auto dialect{std::move(schema.at("$schema")).to_string()};
    dialect.pop_back();
    schema.at("$schema").into(sourcemeta::core::JSON{dialect});
  }
};
