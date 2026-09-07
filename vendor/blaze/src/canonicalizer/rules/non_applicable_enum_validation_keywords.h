class NonApplicableEnumValidationKeywords final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  NonApplicableEnumValidationKeywords()
      : SchemaTransformRule{"non_applicable_enum_validation_keywords"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::SchemaVocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &walker,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(
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
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1_HYPER}) &&
        schema.is_object() && !schema.defines("type"));

    const auto *enum_value{schema.try_at("enum")};
    ONLY_CONTINUE_IF(enum_value && enum_value->is_array());

    sourcemeta::core::JSON::TypeSet enum_types;
    for (const auto &value : enum_value->as_array()) {
      enum_types.set(std::to_underlying(value.type()));
    }

    ONLY_CONTINUE_IF(enum_types.any());

    const bool is_draft3{vocabularies.contains_any(
        {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3_HYPER})};

    std::vector<sourcemeta::core::Pointer> positions;
    for (const auto &entry : schema.as_object()) {
      const auto &metadata = walker(entry.first, vocabularies);

      // If instances is empty (none set), the keyword applies to all types
      if (metadata.instances.none()) {
        continue;
      }

      if (is_draft3 && entry.first == "required" && entry.second.is_boolean()) {
        continue;
      }

      // Check if there's any overlap between keyword's applicable types and
      // enum types
      if ((metadata.instances & enum_types).none()) {
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
