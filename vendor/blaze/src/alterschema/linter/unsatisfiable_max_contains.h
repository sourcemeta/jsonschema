class UnsatisfiableMaxContains final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  UnsatisfiableMaxContains()
      : SchemaTransformRule{
            "unsatisfiable_max_contains",
            "Setting the `maxContains` keyword to a number greater than or "
            "equal to the array upper bound does not add any further "
            "constraint"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Validation,
             Vocabularies::Known::JSON_Schema_2019_09_Validation}) &&
        schema.is_object());

    const auto *max_contains{schema.try_at("maxContains")};
    ONLY_CONTINUE_IF(max_contains && max_contains->is_integer());
    const auto *max_items{schema.try_at("maxItems")};
    ONLY_CONTINUE_IF(max_items && max_items->is_integer() &&
                     max_contains->to_integer() >= max_items->to_integer());
    return APPLIES_TO_KEYWORDS("maxContains", "maxItems");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("maxContains");
  }
};
