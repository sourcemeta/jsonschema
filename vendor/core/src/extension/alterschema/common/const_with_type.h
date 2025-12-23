class ConstWithType final : public SchemaTransformRule {
public:
  ConstWithType()
      : SchemaTransformRule{
            "const_with_type",
            "Setting `type` alongside `const` is considered an anti-pattern, "
            "as the constant already implies its respective type"} {};

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
                     schema.is_object() && schema.defines("type") &&
                     schema.defines("const"));

    const auto current_types{parse_schema_type(schema.at("type"))};
    ONLY_CONTINUE_IF(current_types.test(
        static_cast<std::size_t>(schema.at("const").type())));
    return APPLIES_TO_KEYWORDS("const", "type");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("type");
  }
};
