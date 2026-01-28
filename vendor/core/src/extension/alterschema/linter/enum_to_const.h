class EnumToConst final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
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
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Validation,
                          Vocabularies::Known::JSON_Schema_2019_09_Validation,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6}) &&
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
