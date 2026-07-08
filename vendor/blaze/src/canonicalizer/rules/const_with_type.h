class ConstWithType final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  ConstWithType() : SchemaTransformRule{"const_with_type"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Validation,
                          Vocabularies::Known::JSON_Schema_2019_09_Validation,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6}) &&
                     schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type);
    const auto *const_value{schema.try_at("const")};
    ONLY_CONTINUE_IF(const_value);

    const auto current_types{parse_schema_type(*type)};
    ONLY_CONTINUE_IF(current_types.any());
    ONLY_CONTINUE_IF(
        current_types.test(std::to_underlying(const_value->type())));
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.erase("type");
  }
};
