class PropertyNamesTypeDefault final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  PropertyNamesTypeDefault()
      : SchemaTransformRule{
            "property_names_type_default",
            "Setting the `type` keyword to `string` inside "
            "`propertyNames` does not add any further constraint"} {};

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
                         {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
                          Vocabularies::Known::JSON_Schema_2019_09_Applicator,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6}) &&
                     schema.is_object());

    const auto *property_names{schema.try_at("propertyNames")};
    ONLY_CONTINUE_IF(property_names && property_names->is_object());
    const auto *type{property_names->try_at("type")};
    ONLY_CONTINUE_IF(
        type && ((type->is_string() && type->to_string() == "string") ||
                 (type->is_array() &&
                  std::all_of(type->as_array().begin(), type->as_array().end(),
                              [](const auto &item) {
                                return item.is_string() &&
                                       item.to_string() == "string";
                              }))));
    return APPLIES_TO_POINTERS({{"propertyNames", "type"}});
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.at("propertyNames").erase("type");
  }
};
