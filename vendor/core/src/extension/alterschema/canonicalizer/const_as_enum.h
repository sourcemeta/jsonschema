class ConstAsEnum final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  ConstAsEnum()
      : SchemaTransformRule{"const_as_enum",
                            "Setting `const` is syntax sugar for an "
                            "enumeration of a single value"} {};

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
                     schema.is_object() && schema.defines("const") &&
                     !schema.defines("enum"));
    return APPLIES_TO_KEYWORDS("const");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    auto values = sourcemeta::core::JSON::make_array();
    values.push_back(schema.at("const"));
    schema.at("const").into(std::move(values));
    schema.rename("const", "enum");
  }
};
