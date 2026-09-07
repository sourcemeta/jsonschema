class DuplicateEnumValues final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::false_type;
  DuplicateEnumValues()
      : SchemaTransformRule{"duplicate_enum_values",
                            "Setting duplicate values in `enum` is "
                            "considered an anti-pattern"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::SchemaVocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
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
    return applies_to_keywords("enum");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    // We want to be super careful to maintain the current ordering
    // as we delete the duplicates
    auto &enumeration{schema.at("enum")};
    std::unordered_set<JSON, HashJSON<JSON>> cache;
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
