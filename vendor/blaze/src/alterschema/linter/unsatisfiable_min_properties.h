class UnsatisfiableMinProperties final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  UnsatisfiableMinProperties()
      : SchemaTransformRule{
            "unsatisfiable_min_properties",
            "Setting `minProperties` to a number less than `required` does "
            "not add any further constraint"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
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
    ONLY_CONTINUE_IF(required && required->is_array() && required->unique() &&
                     std::cmp_greater_equal(required->size(),
                                            static_cast<std::uint64_t>(
                                                min_properties->to_integer())));
    return APPLIES_TO_KEYWORDS("minProperties", "required");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("minProperties");
  }
};
