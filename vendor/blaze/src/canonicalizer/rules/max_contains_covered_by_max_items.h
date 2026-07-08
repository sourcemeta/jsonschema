class MaxContainsCoveredByMaxItems final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::false_type;
  MaxContainsCoveredByMaxItems()
      : SchemaTransformRule{"max_contains_covered_by_max_items"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Validation,
             Vocabularies::Known::JSON_Schema_2019_09_Validation}) &&
        schema.is_object());

    const auto *max_contains{schema.try_at("maxContains")};
    ONLY_CONTINUE_IF(max_contains && max_contains->is_integer());
    const auto *max_items{schema.try_at("maxItems")};
    ONLY_CONTINUE_IF(max_items && max_items->is_integer() &&
                     max_contains->to_integer() > max_items->to_integer());
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.assign("maxContains", schema.at("maxItems"));
  }
};
