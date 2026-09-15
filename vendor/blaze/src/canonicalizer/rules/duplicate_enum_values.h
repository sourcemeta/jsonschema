class DuplicateEnumValues final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::false_type;
  DuplicateEnumValues() : SchemaTransformRule{"duplicate_enum_values"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_SCHEMA_2020_12_VALIDATION,
             SchemaVocabularies::Known::JSON_SCHEMA_2019_09_VALIDATION,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_4,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3_HYPER,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_2,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1}) &&
        schema.is_object());

    const auto *enum_value{schema.try_at("enum")};
    ONLY_CONTINUE_IF(enum_value && enum_value->is_array() &&
                     !enum_value->unique());
    // TODO: Highlight which specific entries in `enum` are duplicated
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    // We want to be super careful to maintain the current ordering
    // as we delete the duplicates
    auto &enumeration{schema.at("enum")};
    std::unordered_set<sourcemeta::core::JSON,
                       sourcemeta::core::HashJSON<sourcemeta::core::JSON>>
        cache;
    for (auto iterator = enumeration.as_array().cbegin();
         iterator != enumeration.as_array().cend();) {
      if (cache.contains(*iterator)) {
        iterator = enumeration.erase(iterator);
      } else {
        cache.emplace(*iterator);
        iterator++;
      }
    }
  }
};
