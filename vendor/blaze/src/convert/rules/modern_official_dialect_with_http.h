class ModernOfficialDialectWithHttp final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  ModernOfficialDialectWithHttp()
      : SchemaTransformRule{"modern_official_dialect_with_http"} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const sourcemeta::core::JSON &,
                               const sourcemeta::core::SchemaVocabularies &,
                               const sourcemeta::core::SchemaFrame &,
                               const sourcemeta::core::SchemaFrame::Location &,
                               const sourcemeta::core::SchemaWalker &,
                               const sourcemeta::core::SchemaResolver &) const
      -> bool override {
    ONLY_CONTINUE_IF(schema.is_object());
    const auto *schema_keyword{schema.try_at("$schema")};
    ONLY_CONTINUE_IF(schema_keyword && schema_keyword->is_string());
    const auto &dialect{schema_keyword->to_string()};
    ONLY_CONTINUE_IF(
        dialect == "http://json-schema.org/draft/2019-09/schema" ||
        dialect == "http://json-schema.org/draft/2019-09/schema#" ||
        dialect == "http://json-schema.org/draft/2019-09/hyper-schema" ||
        dialect == "http://json-schema.org/draft/2019-09/hyper-schema#" ||
        dialect == "http://json-schema.org/draft/2020-12/schema" ||
        dialect == "http://json-schema.org/draft/2020-12/schema#" ||
        dialect == "http://json-schema.org/draft/2020-12/hyper-schema" ||
        dialect == "http://json-schema.org/draft/2020-12/hyper-schema#");
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    const auto &old_dialect{schema.at("$schema").to_string()};
    std::string new_dialect{"https://"};
    new_dialect += old_dialect.substr(7);
    schema.at("$schema").into(sourcemeta::core::JSON{new_dialect});
  }
};
