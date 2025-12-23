class NonApplicableTypeSpecificKeywords final : public SchemaTransformRule {
public:
  NonApplicableTypeSpecificKeywords()
      : SchemaTransformRule{"non_applicable_type_specific_keywords",
                            "Avoid keywords that don't apply to the type or "
                            "types that the current subschema expects"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &walker,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(schema.is_object());

    auto current_types{vocabularies.contains_any(
                           {Vocabularies::Known::JSON_Schema_2020_12_Validation,
                            Vocabularies::Known::JSON_Schema_2019_09_Validation,
                            Vocabularies::Known::JSON_Schema_Draft_7,
                            Vocabularies::Known::JSON_Schema_Draft_6,
                            Vocabularies::Known::JSON_Schema_Draft_4,
                            Vocabularies::Known::JSON_Schema_Draft_3,
                            Vocabularies::Known::JSON_Schema_Draft_2,
                            Vocabularies::Known::JSON_Schema_Draft_2_Hyper,
                            Vocabularies::Known::JSON_Schema_Draft_1,
                            Vocabularies::Known::JSON_Schema_Draft_1_Hyper,
                            Vocabularies::Known::JSON_Schema_Draft_0,
                            Vocabularies::Known::JSON_Schema_Draft_0_Hyper}) &&
                               schema.defines("type")
                           ? parse_schema_type(schema.at("type"))
                           : sourcemeta::core::JSON::TypeSet{}};

    if (vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Validation,
             Vocabularies::Known::JSON_Schema_2019_09_Validation,
             Vocabularies::Known::JSON_Schema_Draft_7,
             Vocabularies::Known::JSON_Schema_Draft_6,
             Vocabularies::Known::JSON_Schema_Draft_4,
             Vocabularies::Known::JSON_Schema_Draft_3,
             Vocabularies::Known::JSON_Schema_Draft_2,
             Vocabularies::Known::JSON_Schema_Draft_1}) &&
        schema.defines("enum") && schema.at("enum").is_array()) {
      for (const auto &entry : schema.at("enum").as_array()) {
        current_types.set(static_cast<std::size_t>(entry.type()));
      }
    }

    if (vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Validation,
             Vocabularies::Known::JSON_Schema_2019_09_Validation,
             Vocabularies::Known::JSON_Schema_Draft_7,
             Vocabularies::Known::JSON_Schema_Draft_6}) &&
        schema.defines("const")) {
      current_types.set(static_cast<std::size_t>(schema.at("const").type()));
    }

    // This means that the schema has no explicit type constraints,
    // so we cannot remove anything from it.
    ONLY_CONTINUE_IF(current_types.any());

    std::vector<Pointer> positions;
    for (const auto &entry : schema.as_object()) {
      const auto &metadata{walker(entry.first, vocabularies)};

      // The keyword applies to any type, so it cannot be removed
      if (metadata.instances.none()) {
        continue;
      }

      // If none of the types that the keyword applies to is a valid
      // type for the current schema, then by definition we can remove it
      if ((metadata.instances & current_types).none()) {
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
