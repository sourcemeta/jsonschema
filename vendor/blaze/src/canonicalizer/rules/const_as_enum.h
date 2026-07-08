class ConstAsEnum final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  ConstAsEnum() : SchemaTransformRule{"const_as_enum"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Validation,
                          Vocabularies::Known::JSON_Schema_2019_09_Validation,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6}) &&
                     schema.is_object() && schema.defines("const") &&
                     !schema.defines("enum"));
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    auto values = sourcemeta::core::JSON::make_array();
    values.push_back(schema.at("const"));
    schema.at("const").into(std::move(values));
    schema.rename("const", "enum");
  }
};
