class EqualNumericBoundsToEnum final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  EqualNumericBoundsToEnum()
      : SchemaTransformRule{"equal_numeric_bounds_to_enum"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_4,
                          SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3,
                          SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3_HYPER,
                          SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_2,
                          SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1,
                          SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_0}) &&
                     schema.is_object());

    // An integer only where the dialect reads one as any number whose
    // fractional part is zero. Draft 3 and Draft 4 read it as a number written
    // without a fractional part, so there `type: "integer"` rejects `3.0`
    // where `enum: [ 3 ]` accepts it, the two being equal as numbers. Keeping
    // the type beside the enum is not an option either, as the canonical form
    // has no entry that carries both
    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type && type->is_string() &&
                     (type->to_string() == "number" ||
                      (type->to_string() == "integer" &&
                       integral_reals_are_integers(vocabularies))));
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
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    sourcemeta::core::JSON values = sourcemeta::core::JSON::make_array();
    values.push_back(schema.at("minimum"));
    schema.assign("enum", std::move(values));
    schema.erase("type");
    schema.erase("minimum");
    schema.erase("maximum");
  }
};
