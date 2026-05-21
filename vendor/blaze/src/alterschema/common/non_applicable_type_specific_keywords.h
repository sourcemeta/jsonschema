class NonApplicableTypeSpecificKeywords final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  NonApplicableTypeSpecificKeywords()
      : SchemaTransformRule{"non_applicable_type_specific_keywords",
                            "Avoid keywords that don't apply to the type or "
                            "types that the current subschema expects"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &frame,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &walker,
            const sourcemeta::blaze::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(schema.is_object());

    const auto *type_value{schema.try_at("type")};
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
                               type_value
                           ? parse_schema_type(*type_value)
                           : sourcemeta::core::JSON::TypeSet{}};

    if (vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Validation,
             Vocabularies::Known::JSON_Schema_2019_09_Validation,
             Vocabularies::Known::JSON_Schema_Draft_7,
             Vocabularies::Known::JSON_Schema_Draft_6,
             Vocabularies::Known::JSON_Schema_Draft_4,
             Vocabularies::Known::JSON_Schema_Draft_3,
             Vocabularies::Known::JSON_Schema_Draft_2,
             Vocabularies::Known::JSON_Schema_Draft_1})) {
      const auto *enum_value{schema.try_at("enum")};
      if (enum_value && enum_value->is_array()) {
        for (const auto &entry : enum_value->as_array()) {
          current_types.set(std::to_underlying(entry.type()));
        }
      }
    }

    if (vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Validation,
             Vocabularies::Known::JSON_Schema_2019_09_Validation,
             Vocabularies::Known::JSON_Schema_Draft_7,
             Vocabularies::Known::JSON_Schema_Draft_6})) {
      const auto *const_value{schema.try_at("const")};
      if (const_value) {
        current_types.set(std::to_underlying(const_value->type()));
      }
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

      if (entry.first == "required" &&
          vocabularies.contains_any(
              {Vocabularies::Known::JSON_Schema_Draft_3,
               Vocabularies::Known::JSON_Schema_Draft_3_Hyper})) {
        continue;
      }

      if (entry.first == "maxDecimal" &&
          vocabularies.contains_any(
              {Vocabularies::Known::JSON_Schema_Draft_0,
               Vocabularies::Known::JSON_Schema_Draft_0_Hyper,
               Vocabularies::Known::JSON_Schema_Draft_1,
               Vocabularies::Known::JSON_Schema_Draft_1_Hyper})) {
        continue;
      }

      if (entry.first == "optional" &&
          vocabularies.contains_any(
              {Vocabularies::Known::JSON_Schema_Draft_0,
               Vocabularies::Known::JSON_Schema_Draft_0_Hyper,
               Vocabularies::Known::JSON_Schema_Draft_1,
               Vocabularies::Known::JSON_Schema_Draft_1_Hyper,
               Vocabularies::Known::JSON_Schema_Draft_2,
               Vocabularies::Known::JSON_Schema_Draft_2_Hyper})) {
        continue;
      }

      // If none of the types that the keyword applies to is a valid
      // type for the current schema, then by definition we can remove it
      if ((metadata.instances & current_types).none()) {
        // Skip keywords that have references pointing to them
        if (frame.has_references_through(
                location.pointer, WeakPointer::Token{std::cref(entry.first)})) {
          continue;
        }

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
