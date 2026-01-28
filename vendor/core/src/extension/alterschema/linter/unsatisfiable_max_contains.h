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
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Validation,
             Vocabularies::Known::JSON_Schema_2019_09_Validation}) &&
        schema.is_object() && schema.defines("maxContains") &&
        schema.at("maxContains").is_integer() && schema.defines("maxItems") &&
        schema.at("maxItems").is_integer() &&
        schema.at("maxContains").to_integer() >=
            schema.at("maxItems").to_integer());
    return APPLIES_TO_KEYWORDS("maxContains", "maxItems");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("maxContains");
  }
};
