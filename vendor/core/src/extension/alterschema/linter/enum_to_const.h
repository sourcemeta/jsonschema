class EnumToConst final : public SchemaTransformRule {
public:
  EnumToConst()
      : SchemaTransformRule{
            "enum_to_const",
            "An `enum` of a single value can be expressed as `const`"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        contains_any(vocabularies,
                     {"https://json-schema.org/draft/2020-12/vocab/validation",
                      "https://json-schema.org/draft/2019-09/vocab/validation",
                      "http://json-schema.org/draft-07/schema#",
                      "http://json-schema.org/draft-06/schema#"}) &&
        schema.is_object() && !schema.defines("const") &&
        schema.defines("enum") && schema.at("enum").is_array() &&
        schema.at("enum").size() == 1);
    return APPLIES_TO_KEYWORDS("enum");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    auto front{schema.at("enum").front()};
    schema.at("enum").into(front);
    schema.rename("enum", "const");
  }
};
