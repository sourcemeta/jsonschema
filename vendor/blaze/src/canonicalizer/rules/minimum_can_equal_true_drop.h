class MinimumCanEqualTrueDrop final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  MinimumCanEqualTrueDrop()
      : SchemaTransformRule{"minimum_can_equal_true_drop"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any({Vocabularies::Known::JSON_Schema_Draft_0,
                                   Vocabularies::Known::JSON_Schema_Draft_1,
                                   Vocabularies::Known::JSON_Schema_Draft_2}) &&
        schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(
        type && type->is_string() &&
        (type->to_string() == "integer" || type->to_string() == "number"));
    const auto *minimum_can_equal{schema.try_at("minimumCanEqual")};
    ONLY_CONTINUE_IF(minimum_can_equal && minimum_can_equal->is_boolean() &&
                     minimum_can_equal->to_boolean());
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.erase("minimumCanEqual");
  }
};
