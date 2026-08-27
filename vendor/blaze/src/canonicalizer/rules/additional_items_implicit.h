class AdditionalItemsImplicit final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  AdditionalItemsImplicit()
      : SchemaTransformRule{"additional_items_implicit"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::SchemaVocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {SchemaVocabularies::Known::JSON_Schema_Draft_3,
                          SchemaVocabularies::Known::JSON_Schema_Draft_4,
                          SchemaVocabularies::Known::JSON_Schema_Draft_6,
                          SchemaVocabularies::Known::JSON_Schema_Draft_7}) &&
                     schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type && type->is_string() && type->to_string() == "array");
    const auto *items{schema.try_at("items")};
    ONLY_CONTINUE_IF(items && items->is_array() &&
                     !schema.defines("additionalItems"));
    this->is_draft3_ =
        vocabularies.contains(SchemaVocabularies::Known::JSON_Schema_Draft_3);
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.assign("additionalItems", this->is_draft3_
                                         ? sourcemeta::core::JSON::make_object()
                                         : sourcemeta::core::JSON{true});
  }

private:
  mutable bool is_draft3_{false};
};
