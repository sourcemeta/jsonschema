class NonApplicableTypeSpecificKeywords final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  NonApplicableTypeSpecificKeywords()
      : SchemaTransformRule{"non_applicable_type_specific_keywords"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::SchemaVocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &frame,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &walker,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(schema.is_object());

    const auto *type_value{schema.try_at("type")};
    auto current_types{
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_SCHEMA_2020_12_VALIDATION,
             SchemaVocabularies::Known::JSON_SCHEMA_2019_09_VALIDATION,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_4,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_2,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_2_HYPER,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1_HYPER,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_0,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_0_HYPER}) &&
                (type_value != nullptr) &&
                is_known_type_form(*type_value, vocabularies)
            ? parse_schema_type(*type_value)
            : sourcemeta::core::JSON::TypeSet{}};

    if (vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_SCHEMA_2020_12_VALIDATION,
             SchemaVocabularies::Known::JSON_SCHEMA_2019_09_VALIDATION,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_4,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_2,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1})) {
      const auto *enum_value{schema.try_at("enum")};
      if ((enum_value != nullptr) && enum_value->is_array()) {
        for (const auto &entry : enum_value->as_array()) {
          current_types.set(std::to_underlying(entry.type()));
        }
      }
    }

    if (vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_SCHEMA_2020_12_VALIDATION,
             SchemaVocabularies::Known::JSON_SCHEMA_2019_09_VALIDATION,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6})) {
      const auto *const_value{schema.try_at("const")};
      if (const_value != nullptr) {
        current_types.set(std::to_underlying(const_value->type()));
      }
    }

    // This means that the schema has no explicit type constraints,
    // so we cannot remove anything from it.
    ONLY_CONTINUE_IF(current_types.any());

    std::vector<sourcemeta::core::Pointer> positions;
    for (const auto &entry : schema.as_object()) {
      const auto &metadata{walker(entry.first, vocabularies)};

      // The keyword applies to any type, so it cannot be removed
      if (metadata.instances.none()) {
        continue;
      }

      if (entry.first == "required" &&
          vocabularies.contains_any(
              {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3,
               SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3_HYPER})) {
        continue;
      }

      if (entry.first == "maxDecimal" &&
          vocabularies.contains_any(
              {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_0,
               SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_0_HYPER,
               SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1,
               SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1_HYPER})) {
        continue;
      }

      if (entry.first == "optional" &&
          vocabularies.contains_any(
              {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_0,
               SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_0_HYPER,
               SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1,
               SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1_HYPER,
               SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_2,
               SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_2_HYPER})) {
        continue;
      }

      // If none of the types that the keyword applies to is a valid
      // type for the current schema, then by definition we can remove it
      if ((metadata.instances & current_types).none()) {
        // Skip keywords that have references pointing to them
        if (frame.has_references_through(
                location.pointer,
                sourcemeta::core::WeakPointer::Token{std::cref(entry.first)})) {
          continue;
        }

        positions.push_back(sourcemeta::core::Pointer{entry.first});
      }
    }

    ONLY_CONTINUE_IF(!positions.empty());
    this->locations_ = std::move(positions);
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    for (const auto &location : this->locations_) {
      schema.erase(location.at(0).to_property());
    }
  }

private:
  mutable std::vector<sourcemeta::core::Pointer> locations_;
};
