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
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Validation,
             Vocabularies::Known::JSON_Schema_2019_09_Validation,
             Vocabularies::Known::JSON_Schema_Draft_7,
             Vocabularies::Known::JSON_Schema_Draft_6,
             Vocabularies::Known::JSON_Schema_Draft_4}) &&
        schema.is_object() && schema.defines("minProperties") &&
        schema.at("minProperties").is_integer() && schema.defines("required") &&
        schema.at("required").is_array() && schema.at("required").unique() &&
        std::cmp_greater_equal(schema.at("required").size(),
                               static_cast<std::uint64_t>(
                                   schema.at("minProperties").to_integer())));
    return APPLIES_TO_KEYWORDS("minProperties", "required");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("minProperties");
  }
};
