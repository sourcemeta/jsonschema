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
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Validation,
                          Vocabularies::Known::JSON_Schema_2019_09_Validation,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4,
                          Vocabularies::Known::JSON_Schema_Draft_3,
                          Vocabularies::Known::JSON_Schema_Draft_3_Hyper,
                          Vocabularies::Known::JSON_Schema_Draft_2,
                          Vocabularies::Known::JSON_Schema_Draft_1}) &&
                     schema.is_object());

    const auto *enum_value{schema.try_at("enum")};
    ONLY_CONTINUE_IF(enum_value && enum_value->is_array() &&
                     !enum_value->unique());
    // TODO: Highlight which specific entries in `enum` are duplicated
    return APPLIES_TO_KEYWORDS("enum");
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
