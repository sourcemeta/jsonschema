class MinimumCanEqualTrueDrop final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  MinimumCanEqualTrueDrop()
      : SchemaTransformRule{"minimum_can_equal_true_drop", ""} {};

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

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("minimumCanEqual");
  }
};
