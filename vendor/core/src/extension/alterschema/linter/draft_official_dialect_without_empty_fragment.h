class DraftOfficialDialectWithoutEmptyFragment final
    : public SchemaTransformRule {
public:
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
    if (!schema.is_object() || !schema.defines("$schema") ||
        !schema.at("$schema").is_string()) {
      return false;
    }

    const auto &schema_value = schema.at("$schema").to_string();
    return (schema_value == "http://json-schema.org/draft-07/schema" ||
            schema_value == "http://json-schema.org/draft-07/hyper-schema" ||
            schema_value == "http://json-schema.org/draft-06/schema" ||
            schema_value == "http://json-schema.org/draft-06/hyper-schema" ||
            schema_value == "http://json-schema.org/draft-04/schema" ||
            schema_value == "http://json-schema.org/draft-04/hyper-schema" ||
            schema_value == "http://json-schema.org/draft-03/schema" ||
            schema_value == "http://json-schema.org/draft-03/hyper-schema" ||
            schema_value == "http://json-schema.org/draft-02/schema" ||
            schema_value == "http://json-schema.org/draft-02/hyper-schema" ||
            schema_value == "http://json-schema.org/draft-01/schema" ||
            schema_value == "http://json-schema.org/draft-01/hyper-schema" ||
            schema_value == "http://json-schema.org/draft-00/schema" ||
            schema_value == "http://json-schema.org/draft-00/hyper-schema");
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    auto schema_value = schema.at("$schema").to_string();
    schema_value += "#";
    schema.at("$schema").into(sourcemeta::core::JSON{schema_value});
  }
};
