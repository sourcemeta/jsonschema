class ModernOfficialDialectWithEmptyFragment final
    : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  ModernOfficialDialectWithEmptyFragment()
      : SchemaTransformRule{"modern_official_dialect_with_empty_fragment"} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const sourcemeta::core::JSON &,
                               const sourcemeta::blaze::Vocabularies &,
                               const sourcemeta::blaze::SchemaFrame &,
                               const sourcemeta::blaze::SchemaFrame::Location &,
                               const sourcemeta::blaze::SchemaWalker &,
                               const sourcemeta::blaze::SchemaResolver &) const
      -> bool override {
    ONLY_CONTINUE_IF(schema.is_object());
    const auto *schema_keyword{schema.try_at("$schema")};
    ONLY_CONTINUE_IF(schema_keyword && schema_keyword->is_string());
    const auto &dialect{schema_keyword->to_string()};
    ONLY_CONTINUE_IF(
        dialect == "https://json-schema.org/draft/2019-09/schema#" ||
        dialect == "https://json-schema.org/draft/2019-09/hyper-schema#" ||
        dialect == "https://json-schema.org/draft/2020-12/schema#" ||
        dialect == "https://json-schema.org/draft/2020-12/hyper-schema#");
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    auto dialect{std::move(schema.at("$schema")).to_string()};
    dialect.pop_back();
    schema.at("$schema").into(sourcemeta::core::JSON{dialect});
  }
};
