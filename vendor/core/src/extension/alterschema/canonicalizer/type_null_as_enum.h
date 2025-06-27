class TypeNullAsEnum final : public SchemaTransformRule {
public:
  TypeNullAsEnum()
      : SchemaTransformRule{
            "type_null_as_enum",
            "Setting `type` to `null` is syntax sugar for an enumeration "
            "of a single value: `null`"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    return contains_any(
               vocabularies,
               {"https://json-schema.org/draft/2020-12/vocab/validation",
                "https://json-schema.org/draft/2019-09/vocab/validation",
                "http://json-schema.org/draft-07/schema#",
                "http://json-schema.org/draft-06/schema#",
                "http://json-schema.org/draft-04/schema#",
                "http://json-schema.org/draft-03/schema#",
                "http://json-schema.org/draft-02/schema#",
                "http://json-schema.org/draft-01/schema#"}) &&
           schema.is_object() && schema.defines("type") &&
           schema.at("type").is_string() &&
           schema.at("type").to_string() == "null" && !schema.defines("enum") &&
           !schema.defines("const");
  }

  auto transform(JSON &schema) const -> void override {
    auto choices = sourcemeta::core::JSON::make_array();
    choices.push_back(sourcemeta::core::JSON{nullptr});
    schema.at("type").into(std::move(choices));
    schema.rename("type", "enum");
  }
};
