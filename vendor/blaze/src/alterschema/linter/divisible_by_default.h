class DivisibleByDefault final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  DivisibleByDefault()
      : SchemaTransformRule{
            "divisible_by_default",
            "Setting `divisibleBy` to 1 does not add any further constraint"} {
        };

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_Draft_3,
                          Vocabularies::Known::JSON_Schema_Draft_3_Hyper}) &&
                     schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type && type->is_string() &&
                     type->to_string() == "integer");

    const auto *divisible_by{schema.try_at("divisibleBy")};
    ONLY_CONTINUE_IF(
        divisible_by &&
        ((divisible_by->is_integer() && divisible_by->to_integer() == 1) ||
         (divisible_by->is_real() && divisible_by->to_real() == 1.0) ||
         (divisible_by->is_decimal() &&
          divisible_by->to_decimal() == sourcemeta::core::Decimal{1})));
    return APPLIES_TO_KEYWORDS("divisibleBy");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("divisibleBy");
  }
};
