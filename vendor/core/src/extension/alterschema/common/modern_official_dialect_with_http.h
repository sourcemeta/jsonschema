class ModernOfficialDialectWithHttp final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  ModernOfficialDialectWithHttp()
      : SchemaTransformRule{
            "modern_official_dialect_with_http",
            "The official dialect URI of 2019-09 and later must use "
            "\"https://\" instead of \"http://\""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    using sourcemeta::core::SchemaBaseDialect;
    ONLY_CONTINUE_IF(
        location.base_dialect == SchemaBaseDialect::JSON_Schema_2020_12 ||
        location.base_dialect == SchemaBaseDialect::JSON_Schema_2020_12_Hyper ||
        location.base_dialect == SchemaBaseDialect::JSON_Schema_2019_09 ||
        location.base_dialect == SchemaBaseDialect::JSON_Schema_2019_09_Hyper);
    ONLY_CONTINUE_IF(schema.is_object() && schema.defines("$schema") &&
                     schema.at("$schema").is_string());
    const auto &dialect{schema.at("$schema").to_string()};
    ONLY_CONTINUE_IF(dialect.starts_with("http://json-schema.org/"));
    ONLY_CONTINUE_IF(
        dialect == "http://json-schema.org/draft/2020-12/schema" ||
        dialect == "http://json-schema.org/draft/2020-12/schema#" ||
        dialect == "http://json-schema.org/draft/2020-12/hyper-schema" ||
        dialect == "http://json-schema.org/draft/2020-12/hyper-schema#" ||
        dialect == "http://json-schema.org/draft/2019-09/schema" ||
        dialect == "http://json-schema.org/draft/2019-09/schema#" ||
        dialect == "http://json-schema.org/draft/2019-09/hyper-schema" ||
        dialect == "http://json-schema.org/draft/2019-09/hyper-schema#");
    return APPLIES_TO_KEYWORDS("$schema");
  }

  auto transform(sourcemeta::core::JSON &schema, const Result &) const
      -> void override {
    const auto &old_dialect{schema.at("$schema").to_string()};
    std::string new_dialect{"https://"};
    new_dialect += old_dialect.substr(7);
    schema.at("$schema").into(sourcemeta::core::JSON{new_dialect});
  }
};
