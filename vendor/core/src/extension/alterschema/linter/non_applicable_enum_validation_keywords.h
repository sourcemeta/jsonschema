class NonApplicableEnumValidationKeywords final : public SchemaTransformRule {
public:
  NonApplicableEnumValidationKeywords()
      : SchemaTransformRule{
            "non_applicable_enum_validation_keywords",
            "Setting validation keywords that do not apply to any item in "
            "`enum` is considered an anti-pattern"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &walker,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        contains_any(vocabularies,
                     {"https://json-schema.org/draft/2020-12/vocab/validation",
                      "https://json-schema.org/draft/2019-09/vocab/validation",
                      "http://json-schema.org/draft-07/schema#",
                      "http://json-schema.org/draft-06/schema#",
                      "http://json-schema.org/draft-04/schema#",
                      "http://json-schema.org/draft-03/schema#",
                      "http://json-schema.org/draft-02/schema#",
                      "http://json-schema.org/draft-02/hyper-schema#",
                      "http://json-schema.org/draft-01/schema#",
                      "http://json-schema.org/draft-01/hyper-schema#"}) &&
        schema.is_object() && schema.defines("enum") &&
        schema.at("enum").is_array());

    std::set<sourcemeta::core::JSON::Type> enum_types;
    for (const auto &value : schema.at("enum").as_array()) {
      enum_types.emplace(value.type());
    }

    ONLY_CONTINUE_IF(!enum_types.empty());

    std::vector<Pointer> positions;
    for (const auto &entry : schema.as_object()) {
      const auto metadata = walker(entry.first, vocabularies);

      if (metadata.instances.empty()) {
        continue;
      }

      if (std::ranges::none_of(metadata.instances,
                               [&enum_types](const auto keyword_type) {
                                 return enum_types.contains(keyword_type);
                               })) {
        positions.push_back(Pointer{entry.first});
      }
    }

    ONLY_CONTINUE_IF(!positions.empty());

    return APPLIES_TO_POINTERS(std::move(positions));
  }

  auto transform(JSON &schema, const Result &result) const -> void override {
    for (const auto &location : result.locations) {
      schema.erase(location.at(0).to_property());
    }
  }
};
