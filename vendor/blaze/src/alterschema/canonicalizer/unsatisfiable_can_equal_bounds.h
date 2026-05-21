class UnsatisfiableCanEqualBounds final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::false_type;
  UnsatisfiableCanEqualBounds()
      : SchemaTransformRule{"unsatisfiable_can_equal_bounds", ""} {};

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
        vocabularies.contains(Vocabularies::Known::JSON_Schema_Draft_2) &&
        schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(
        type && type->is_string() &&
        (type->to_string() == "number" || type->to_string() == "integer"));
    const auto *minimum{schema.try_at("minimum")};
    ONLY_CONTINUE_IF(minimum && minimum->is_number());
    const auto *maximum{schema.try_at("maximum")};
    ONLY_CONTINUE_IF(maximum && maximum->is_number() && *minimum == *maximum);

    const auto *minimum_can_equal{schema.try_at("minimumCanEqual")};
    const bool min_exclusive{minimum_can_equal &&
                             minimum_can_equal->is_boolean() &&
                             !minimum_can_equal->to_boolean()};
    const auto *maximum_can_equal{schema.try_at("maximumCanEqual")};
    const bool max_exclusive{maximum_can_equal &&
                             maximum_can_equal->is_boolean() &&
                             !maximum_can_equal->to_boolean()};
    ONLY_CONTINUE_IF(min_exclusive || max_exclusive);
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.into(JSON{false});
  }
};
