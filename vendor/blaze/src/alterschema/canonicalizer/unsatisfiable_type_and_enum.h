class UnsatisfiableTypeAndEnum final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::false_type;
  UnsatisfiableTypeAndEnum()
      : SchemaTransformRule{"unsatisfiable_type_and_enum", ""} {};

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
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_Draft_0,
             Vocabularies::Known::JSON_Schema_Draft_1,
             Vocabularies::Known::JSON_Schema_Draft_2,
             Vocabularies::Known::JSON_Schema_Draft_3,
             Vocabularies::Known::JSON_Schema_Draft_4,
             Vocabularies::Known::JSON_Schema_Draft_6,
             Vocabularies::Known::JSON_Schema_Draft_7,
             Vocabularies::Known::JSON_Schema_2019_09_Validation,
             Vocabularies::Known::JSON_Schema_2020_12_Validation}) &&
        schema.is_object() && schema.defines("type") &&
        schema.at("type").is_string() && schema.defines("enum") &&
        schema.at("enum").is_array() && !schema.at("enum").empty());

    const auto declared_types{parse_schema_type(schema.at("type"))};
    const bool integer_matches_integral{
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_Draft_6,
             Vocabularies::Known::JSON_Schema_Draft_7,
             Vocabularies::Known::JSON_Schema_2019_09_Validation,
             Vocabularies::Known::JSON_Schema_2020_12_Validation}) &&
        declared_types.test(std::to_underlying(JSON::Type::Integer))};
    ONLY_CONTINUE_IF(std::ranges::none_of(
        schema.at("enum").as_array(),
        [&declared_types, integer_matches_integral](const auto &value) {
          return declared_types.test(std::to_underlying(value.type())) ||
                 (integer_matches_integral && value.is_integral());
        }));
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.into(JSON{false});
  }
};
