class MultipleOfImplicit final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  MultipleOfImplicit() : SchemaTransformRule{"multiple_of_implicit"} {};

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
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4}) &&
                     schema.is_object() && !schema.defines("multipleOf"));

    const auto *type{schema.try_at("type")};
    // Applying this to numbers would be a semantic problem
    ONLY_CONTINUE_IF(type && type->is_string() &&
                     type->to_string() == "integer");
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.assign("multipleOf", sourcemeta::core::JSON{1});
  }
};
