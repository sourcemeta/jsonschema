class DraftOfficialDialectWithHttps final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  DraftOfficialDialectWithHttps()
      : SchemaTransformRule{
            "draft_official_dialect_with_https",
            "The official dialect URI of Draft 7 and older must use "
            "\"http://\" instead of \"https://\""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    using sourcemeta::core::SchemaBaseDialect;
    ONLY_CONTINUE_IF(
        location.base_dialect == SchemaBaseDialect::JSON_Schema_Draft_7 ||
        location.base_dialect == SchemaBaseDialect::JSON_Schema_Draft_7_Hyper ||
        location.base_dialect == SchemaBaseDialect::JSON_Schema_Draft_6 ||
        location.base_dialect == SchemaBaseDialect::JSON_Schema_Draft_6_Hyper ||
        location.base_dialect == SchemaBaseDialect::JSON_Schema_Draft_4 ||
        location.base_dialect == SchemaBaseDialect::JSON_Schema_Draft_4_Hyper ||
        location.base_dialect == SchemaBaseDialect::JSON_Schema_Draft_3 ||
        location.base_dialect == SchemaBaseDialect::JSON_Schema_Draft_3_Hyper ||
        location.base_dialect == SchemaBaseDialect::JSON_Schema_Draft_2_Hyper ||
        location.base_dialect == SchemaBaseDialect::JSON_Schema_Draft_1_Hyper ||
        location.base_dialect == SchemaBaseDialect::JSON_Schema_Draft_0_Hyper);
    ONLY_CONTINUE_IF(schema.is_object() && schema.defines("$schema") &&
                     schema.at("$schema").is_string());
    const auto &dialect{schema.at("$schema").to_string()};
    ONLY_CONTINUE_IF(dialect.starts_with("https://json-schema.org/"));
    ONLY_CONTINUE_IF(
        dialect == "https://json-schema.org/draft-07/schema" ||
        dialect == "https://json-schema.org/draft-07/schema#" ||
        dialect == "https://json-schema.org/draft-07/hyper-schema" ||
        dialect == "https://json-schema.org/draft-07/hyper-schema#" ||
        dialect == "https://json-schema.org/draft-06/schema" ||
        dialect == "https://json-schema.org/draft-06/schema#" ||
        dialect == "https://json-schema.org/draft-06/hyper-schema" ||
        dialect == "https://json-schema.org/draft-06/hyper-schema#" ||
        dialect == "https://json-schema.org/draft-04/schema" ||
        dialect == "https://json-schema.org/draft-04/schema#" ||
        dialect == "https://json-schema.org/draft-04/hyper-schema" ||
        dialect == "https://json-schema.org/draft-04/hyper-schema#" ||
        dialect == "https://json-schema.org/draft-03/schema" ||
        dialect == "https://json-schema.org/draft-03/schema#" ||
        dialect == "https://json-schema.org/draft-03/hyper-schema" ||
        dialect == "https://json-schema.org/draft-03/hyper-schema#" ||
        dialect == "https://json-schema.org/draft-02/schema" ||
        dialect == "https://json-schema.org/draft-02/schema#" ||
        dialect == "https://json-schema.org/draft-02/hyper-schema" ||
        dialect == "https://json-schema.org/draft-02/hyper-schema#" ||
        dialect == "https://json-schema.org/draft-01/schema" ||
        dialect == "https://json-schema.org/draft-01/schema#" ||
        dialect == "https://json-schema.org/draft-01/hyper-schema" ||
        dialect == "https://json-schema.org/draft-01/hyper-schema#" ||
        dialect == "https://json-schema.org/draft-00/schema" ||
        dialect == "https://json-schema.org/draft-00/schema#" ||
        dialect == "https://json-schema.org/draft-00/hyper-schema" ||
        dialect == "https://json-schema.org/draft-00/hyper-schema#");
    return APPLIES_TO_KEYWORDS("$schema");
  }

  auto transform(sourcemeta::core::JSON &schema, const Result &) const
      -> void override {
    const auto &old_dialect{schema.at("$schema").to_string()};
    std::string new_dialect{"http://"};
    new_dialect += old_dialect.substr(8);
    schema.at("$schema").into(sourcemeta::core::JSON{new_dialect});
  }
};
