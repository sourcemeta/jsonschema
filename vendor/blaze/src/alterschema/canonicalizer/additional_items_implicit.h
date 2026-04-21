class AdditionalItemsImplicit final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  AdditionalItemsImplicit()
      : SchemaTransformRule{"additional_items_implicit", ""} {};

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
        vocabularies.contains_any({Vocabularies::Known::JSON_Schema_Draft_3,
                                   Vocabularies::Known::JSON_Schema_Draft_4,
                                   Vocabularies::Known::JSON_Schema_Draft_6,
                                   Vocabularies::Known::JSON_Schema_Draft_7}) &&
        schema.is_object() && schema.defines("type") &&
        schema.at("type").is_string() &&
        schema.at("type").to_string() == "array" && schema.defines("items") &&
        schema.at("items").is_array() && !schema.defines("additionalItems"));
    this->is_draft3_ =
        vocabularies.contains(Vocabularies::Known::JSON_Schema_Draft_3);
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.assign("additionalItems", this->is_draft3_
                                         ? sourcemeta::core::JSON::make_object()
                                         : sourcemeta::core::JSON{true});
  }

private:
  mutable bool is_draft3_{false};
};
