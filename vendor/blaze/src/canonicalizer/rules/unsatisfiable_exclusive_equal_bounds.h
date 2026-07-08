class UnsatisfiableExclusiveEqualBounds final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::false_type;
  UnsatisfiableExclusiveEqualBounds()
      : SchemaTransformRule{"unsatisfiable_exclusive_equal_bounds"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any({Vocabularies::Known::JSON_Schema_Draft_3,
                                   Vocabularies::Known::JSON_Schema_Draft_4}) &&
        schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(
        type && type->is_string() &&
        (type->to_string() == "number" || type->to_string() == "integer"));
    const auto *minimum{schema.try_at("minimum")};
    ONLY_CONTINUE_IF(minimum && minimum->is_number());
    const auto *maximum{schema.try_at("maximum")};
    ONLY_CONTINUE_IF(maximum && maximum->is_number() && *minimum == *maximum);

    const auto *exclusive_minimum{schema.try_at("exclusiveMinimum")};
    const bool exclusive_min{exclusive_minimum &&
                             exclusive_minimum->is_boolean() &&
                             exclusive_minimum->to_boolean()};
    const auto *exclusive_maximum{schema.try_at("exclusiveMaximum")};
    const bool exclusive_max{exclusive_maximum &&
                             exclusive_maximum->is_boolean() &&
                             exclusive_maximum->to_boolean()};
    ONLY_CONTINUE_IF(exclusive_min || exclusive_max);
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.into(sourcemeta::core::JSON{false});
  }
};
