class DraftOfficialDialectWithoutEmptyFragment final
    : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  DraftOfficialDialectWithoutEmptyFragment()
      : SchemaTransformRule{"draft_official_dialect_without_empty_fragment",
                            "The official dialect URI of Draft 7 and older "
                            "versions must contain the empty fragment"} {};

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
    return APPLIES_TO_KEYWORDS("$schema");
  }

  auto transform(sourcemeta::core::JSON &schema, const Result &) const
      -> void override {
    auto dialect{std::move(schema.at("$schema")).to_string()};
    dialect += "#";
    schema.at("$schema").into(sourcemeta::core::JSON{dialect});
  }
};
