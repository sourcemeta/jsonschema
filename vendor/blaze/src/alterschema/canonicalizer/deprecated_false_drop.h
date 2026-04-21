class DeprecatedFalseDrop final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  DeprecatedFalseDrop() : SchemaTransformRule{"deprecated_false_drop", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2019_09_Meta_Data,
             Vocabularies::Known::JSON_Schema_2020_12_Meta_Data}) &&
        schema.is_object() && schema.defines("deprecated") &&
        schema.at("deprecated").is_boolean() &&
        !schema.at("deprecated").to_boolean());
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("deprecated");
  }
};
