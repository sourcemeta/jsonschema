class RequiredPropertiesInProperties final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  RequiredPropertiesInProperties()
      : SchemaTransformRule{"required_properties_in_properties"} {};

  [[nodiscard]] auto condition(
      const sourcemeta::core::JSON &schema, const sourcemeta::core::JSON &root,
      const sourcemeta::core::SchemaVocabularies &vocabularies,
      const sourcemeta::core::SchemaFrame &frame,
      const sourcemeta::core::SchemaFrame::Location &location,
      const sourcemeta::core::SchemaWalker &walker,
      const sourcemeta::core::SchemaResolver &resolver) const -> bool override {
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

    std::vector<sourcemeta::core::Pointer> locations;
    std::size_t index{0};
    for (const auto &property : required->as_array()) {
      if (property.is_string() &&
          !this->defined_in_properties_sibling(schema, property.to_string()) &&
          !walk_up_in_place_applicators(
               root, frame, location, walker, resolver,
               [&](const sourcemeta::core::JSON &ancestor,
                   const SchemaVocabularies &) -> bool {
                 return this->defined_in_properties_sibling(
                     ancestor, property.to_string());
               })
               .has_value()) {
        locations.push_back(sourcemeta::core::Pointer{"required", index});
      }

      index += 1;
    }

    ONLY_CONTINUE_IF(!locations.empty());
    this->locations_ = std::move(locations);
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.assign_if_missing("properties",
                             sourcemeta::core::JSON::make_object());
    for (const auto &location : this->locations_) {
      const auto &property{
          schema.at("required").at(location.at(1).to_index()).to_string()};
      // Draft 4 and earlier have no boolean schemas, and the empty object is
      // the canonical way to say "unconstrained" in every dialect
      schema.at("properties")
          .assign(property, sourcemeta::core::JSON::make_object());
    }
  }

private:
  [[nodiscard]] auto defined_in_properties_sibling(
      const sourcemeta::core::JSON &schema,
      const sourcemeta::core::JSON::String &property) const -> bool {
    assert(schema.is_object());
    const auto *properties{schema.try_at("properties")};
    return (properties != nullptr) && properties->is_object() &&
           properties->defines(property);
  };

  mutable std::vector<sourcemeta::core::Pointer> locations_;
};
