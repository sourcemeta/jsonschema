class MinPropertiesCoveredByRequired final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::false_type;
  MinPropertiesCoveredByRequired()
      : SchemaTransformRule{"min_properties_covered_by_required"} {};

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
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4}) &&
                     schema.is_object());

    const auto *min_properties{schema.try_at("minProperties")};
    ONLY_CONTINUE_IF(min_properties && min_properties->is_integer());
    const auto *required{schema.try_at("required")};
    ONLY_CONTINUE_IF(
        required && required->is_array() && required->unique() &&
        std::cmp_greater(required->size(), static_cast<std::uint64_t>(
                                               min_properties->to_integer())));
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.assign("minProperties",
                  sourcemeta::core::JSON{schema.at("required").size()});
  }
};
