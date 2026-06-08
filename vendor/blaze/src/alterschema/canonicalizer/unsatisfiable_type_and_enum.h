class UnsatisfiableTypeAndEnum final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::false_type;
  UnsatisfiableTypeAndEnum()
      : SchemaTransformRule{"unsatisfiable_type_and_enum", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
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
        schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type && type->is_string());
    const auto *enum_value{schema.try_at("enum")};
    ONLY_CONTINUE_IF(enum_value && enum_value->is_array() &&
                     !enum_value->empty());

    const auto declared_types{parse_schema_type(*type)};
    ONLY_CONTINUE_IF(declared_types.any());
    const bool integer_matches_integral{
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_Draft_6,
             Vocabularies::Known::JSON_Schema_Draft_7,
             Vocabularies::Known::JSON_Schema_2019_09_Validation,
             Vocabularies::Known::JSON_Schema_2020_12_Validation}) &&
        declared_types.test(std::to_underlying(JSON::Type::Integer))};
    ONLY_CONTINUE_IF(std::ranges::none_of(
        enum_value->as_array(),
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
