class ConstNotInEnum final : public SchemaTransformRule {
public:
  using mutates = std::false_type;
  using reframe_after_transform = std::false_type;
  ConstNotInEnum()
      : SchemaTransformRule{
            "const_not_in_enum",
            "Do not set the `const` and `enum` keyword at the same time, "
            "mainly when their values diverge"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Validation,
                          Vocabularies::Known::JSON_Schema_2019_09_Validation,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6}) &&
                     schema.is_object() && schema.defines("const") &&
                     schema.defines("enum") && schema.at("enum").is_array() &&
                     !schema.at("enum").contains(schema.at("const")));
    return APPLIES_TO_KEYWORDS("const", "enum");
  }
};
