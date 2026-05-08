class EqualNumericBoundsToEnum final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  EqualNumericBoundsToEnum()
      : SchemaTransformRule{
            "equal_numeric_bounds_to_enum",
            "Setting `minimum` and `maximum` to the same number only leaves "
            "one possible value"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_Draft_4,
                          Vocabularies::Known::JSON_Schema_Draft_3,
                          Vocabularies::Known::JSON_Schema_Draft_3_Hyper,
                          Vocabularies::Known::JSON_Schema_Draft_2,
                          Vocabularies::Known::JSON_Schema_Draft_1,
                          Vocabularies::Known::JSON_Schema_Draft_0}) &&
                     schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(
        type && type->is_string() &&
        (type->to_string() == "integer" || type->to_string() == "number"));
    const auto *minimum{schema.try_at("minimum")};
    ONLY_CONTINUE_IF(minimum && minimum->is_number());
    const auto *maximum{schema.try_at("maximum")};
    ONLY_CONTINUE_IF(maximum && maximum->is_number() && *minimum == *maximum);

    const auto *exclusive_minimum{schema.try_at("exclusiveMinimum")};
    ONLY_CONTINUE_IF(!(exclusive_minimum && exclusive_minimum->is_boolean() &&
                       exclusive_minimum->to_boolean()));
    const auto *exclusive_maximum{schema.try_at("exclusiveMaximum")};
    ONLY_CONTINUE_IF(!(exclusive_maximum && exclusive_maximum->is_boolean() &&
                       exclusive_maximum->to_boolean()));
    const auto *minimum_can_equal{schema.try_at("minimumCanEqual")};
    ONLY_CONTINUE_IF(!(minimum_can_equal && minimum_can_equal->is_boolean() &&
                       !minimum_can_equal->to_boolean()));
    const auto *maximum_can_equal{schema.try_at("maximumCanEqual")};
    ONLY_CONTINUE_IF(!(maximum_can_equal && maximum_can_equal->is_boolean() &&
                       !maximum_can_equal->to_boolean()));
    return APPLIES_TO_KEYWORDS("minimum", "maximum");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    sourcemeta::core::JSON values = sourcemeta::core::JSON::make_array();
    values.push_back(schema.at("minimum"));
    schema.assign("enum", std::move(values));
    schema.erase("type");
    schema.erase("minimum");
    schema.erase("maximum");
  }
};
