class DraftOfficialDialectWithoutEmptyFragment final
    : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  DraftOfficialDialectWithoutEmptyFragment()
      : SchemaTransformRule{"draft_official_dialect_without_empty_fragment"} {};

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
        dialect == "http://json-schema.org/draft-07/schema" ||
        dialect == "http://json-schema.org/draft-07/hyper-schema" ||
        dialect == "http://json-schema.org/draft-06/schema" ||
        dialect == "http://json-schema.org/draft-06/hyper-schema" ||
        dialect == "http://json-schema.org/draft-04/schema" ||
        dialect == "http://json-schema.org/draft-04/hyper-schema" ||
        dialect == "http://json-schema.org/draft-03/schema" ||
        dialect == "http://json-schema.org/draft-03/hyper-schema" ||
        dialect == "http://json-schema.org/draft-02/schema" ||
        dialect == "http://json-schema.org/draft-02/hyper-schema" ||
        dialect == "http://json-schema.org/draft-01/schema" ||
        dialect == "http://json-schema.org/draft-01/hyper-schema" ||
        dialect == "http://json-schema.org/draft-00/schema" ||
        dialect == "http://json-schema.org/draft-00/hyper-schema");
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    auto dialect{std::move(schema.at("$schema")).to_string()};
    dialect += "#";
    schema.at("$schema").into(sourcemeta::core::JSON{dialect});
  }
};
