class EnumWithType final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  EnumWithType()
      : SchemaTransformRule{
            "enum_with_type",
            "Setting `type` alongside `enum` is considered an anti-pattern, as "
            "the enumeration choices already imply their respective types"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::SchemaVocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
        {SchemaVocabularies::Known::JSON_SCHEMA_2020_12_VALIDATION,
         SchemaVocabularies::Known::JSON_SCHEMA_2019_09_VALIDATION,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_4,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3_HYPER,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_2,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1}));
    ONLY_CONTINUE_IF(schema.is_object());
    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type);
    const auto *enum_value{schema.try_at("enum")};
    ONLY_CONTINUE_IF(enum_value && enum_value->is_array());

    if (vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3_HYPER})) {
      if (type->is_string() && type->to_string() == "any") {
        return applies_to_keywords("enum", "type");
      }

      if (type->is_array()) {
        bool has_tautology{false};
        bool has_unknown_subschema{false};
        for (const auto &entry : type->as_array()) {
          if (entry.is_string() && entry.to_string() == "any") {
            has_tautology = true;
            break;
          }
          if (entry.is_object()) {
            if (entry.empty()) {
              has_tautology = true;
              break;
            }
            has_unknown_subschema = true;
          }
        }

        if (has_tautology) {
          return applies_to_keywords("enum", "type");
        }

        if (has_unknown_subschema) {
          return false;
        }
      }
    }

    const auto current_types{parse_schema_type(*type)};
    ONLY_CONTINUE_IF(current_types.any());
    const bool integer_matches_integral{
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7}) &&
        current_types.test(std::to_underlying(JSON::Type::Integer))};
    ONLY_CONTINUE_IF(std::ranges::all_of(
        enum_value->as_array(),
        [&current_types, integer_matches_integral](const auto &item) -> auto {
          return current_types.test(std::to_underlying(item.type())) ||
                 (integer_matches_integral && item.is_integral());
        }));

    return applies_to_keywords("enum", "type");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("type");
  }
};
