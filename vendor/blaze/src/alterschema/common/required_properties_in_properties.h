class RequiredPropertiesInProperties final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  RequiredPropertiesInProperties()
      : SchemaTransformRule{
            "required_properties_in_properties",
            "Every property listed in the `required` keyword must be "
            "explicitly defined using the `properties` keyword"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &root,
            const sourcemeta::blaze::SchemaVocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &frame,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &walker,
            const sourcemeta::blaze::SchemaResolver &resolver, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        ((vocabularies.contains(
              SchemaVocabularies::Known::JSON_SCHEMA_2020_12_VALIDATION) &&
          vocabularies.contains(
              SchemaVocabularies::Known::JSON_SCHEMA_2020_12_APPLICATOR)) ||
         (vocabularies.contains(
              SchemaVocabularies::Known::JSON_SCHEMA_2019_09_VALIDATION) &&
          vocabularies.contains(
              SchemaVocabularies::Known::JSON_SCHEMA_2019_09_APPLICATOR)) ||
         vocabularies.contains_any(
             {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7,
              SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6,
              SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_4})) &&
        schema.is_object());

    const auto *required{schema.try_at("required")};
    ONLY_CONTINUE_IF(required && required->is_array() && !required->empty());

    const auto *additional_properties{schema.try_at("additionalProperties")};
    ONLY_CONTINUE_IF(!additional_properties ||
                     (additional_properties->is_boolean() &&
                      additional_properties->to_boolean()));

    std::vector<Pointer> locations;
    std::size_t index{0};
    for (const auto &property : required->as_array()) {
      if (property.is_string() &&
          !this->defined_in_properties_sibling(schema, property.to_string()) &&
          !walk_up_in_place_applicators(
               root, frame, location, walker, resolver,
               [&](const JSON &ancestor, const SchemaVocabularies &) -> bool {
                 return this->defined_in_properties_sibling(
                     ancestor, property.to_string());
               })
               .has_value()) {
        locations.push_back(Pointer{"required", index});
      }

      index += 1;
    }

    ONLY_CONTINUE_IF(!locations.empty());
    return applies_to_pointers(std::move(locations));
  }

  auto transform(JSON &schema, const Result &result) const -> void override {
    schema.assign_if_missing("properties",
                             sourcemeta::core::JSON::make_object());
    for (const auto &location : result.locations) {
      const auto &property{
          schema.at("required").at(location.at(1).to_index()).to_string()};
      schema.at("properties").assign(property, sourcemeta::core::JSON{true});
    }
  }

private:
  [[nodiscard]] auto
  defined_in_properties_sibling(const JSON &schema,
                                const JSON::String &property) const -> bool {
    assert(schema.is_object());
    const auto *properties{schema.try_at("properties")};
    return (properties != nullptr) && properties->is_object() &&
           properties->defines(property);
  };
};
