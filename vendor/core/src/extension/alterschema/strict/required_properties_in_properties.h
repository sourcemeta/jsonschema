class RequiredPropertiesInProperties final : public SchemaTransformRule {
public:
  RequiredPropertiesInProperties()
      : SchemaTransformRule{
            "strict/required_properties_in_properties",
            "Every property listed in the `required` keyword must be "
            "explicitly defined using the `properties` keyword"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &root,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &walker,
            const sourcemeta::core::SchemaResolver &resolver) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    if (!(((vocabularies.contains(
                "https://json-schema.org/draft/2020-12/vocab/validation") &&
            vocabularies.contains(
                "https://json-schema.org/draft/2020-12/vocab/applicator")) ||
           (vocabularies.contains(
                "https://json-schema.org/draft/2019-09/vocab/validation") &&
            vocabularies.contains(
                "https://json-schema.org/draft/2019-09/vocab/applicator")) ||
           contains_any(vocabularies,
                        {"http://json-schema.org/draft-07/schema#",
                         "http://json-schema.org/draft-06/schema#",
                         "http://json-schema.org/draft-04/schema#",
                         "http://json-schema.org/draft-03/schema#"})) &&
          schema.is_object() && schema.defines("required") &&
          schema.at("required").is_array() && !schema.at("required").empty())) {
      return false;
    }

    this->properties.clear();
    std::ostringstream message;
    for (const auto &property : schema.at("required").as_array()) {
      if (property.is_string() &&
          !this->defined_in_properties_sibling(schema, property.to_string()) &&
          !this->defined_in_properties_parent(root, frame, location, walker,
                                              resolver, property.to_string())) {
        message << "- " << property.to_string() << "\n";
        this->properties.emplace_back(property.to_string());
      }
    }

    if (this->properties.empty()) {
      return false;
    } else {
      return message.str();
    }
  }

  auto transform(JSON &schema) const -> void override {
    schema.assign_if_missing("properties",
                             sourcemeta::core::JSON::make_object());
    for (const auto &property : this->properties) {
      schema.at("properties")
          .assign(property.get(), sourcemeta::core::JSON{true});
    }
  }

private:
  mutable std::vector<std::reference_wrapper<const JSON::String>> properties;

  [[nodiscard]] auto
  defined_in_properties_sibling(const JSON &schema,
                                const JSON::String &property) const -> bool {
    assert(schema.is_object());
    return schema.defines("properties") &&
           schema.at("properties").is_object() &&
           schema.at("properties").defines(property);
  };

  [[nodiscard]] auto
  defined_in_properties_parent(const JSON &root, const SchemaFrame &frame,
                               const SchemaFrame::Location &location,
                               const SchemaWalker &walker,
                               const SchemaResolver &resolver,
                               const JSON::String &property) const -> bool {
    if (location.parent.has_value()) {
      const auto relative_pointer{
          location.pointer.resolve_from(location.parent.value())};
      assert(!relative_pointer.empty() && relative_pointer.at(0).is_property());
      const auto parent{
          frame.traverse(frame.uri(location.parent.value()).value().get())};
      assert(parent.has_value());
      const auto type{walker(relative_pointer.at(0).to_property(),
                             frame.vocabularies(parent.value().get(), resolver))
                          .type};
      if (type == SchemaKeywordType::ApplicatorElementsInPlaceSome ||
          type == SchemaKeywordType::ApplicatorElementsInPlace ||
          type == SchemaKeywordType::ApplicatorValueInPlaceMaybe ||
          type == SchemaKeywordType::ApplicatorValueInPlaceNegate ||
          type == SchemaKeywordType::ApplicatorValueInPlaceOther) {
        return this->defined_in_properties_sibling(
            get(root, location.parent.value()), property);
      }
    }

    return false;
  };
};
