class SimplePropertiesIdentifiers final : public SchemaTransformRule {
public:
  using mutates = std::false_type;
  using reframe_after_transform = std::false_type;
  SimplePropertiesIdentifiers()
      // Inspired by
      // https://json-structure.github.io/core/draft-vasters-json-structure-core.html#section-3.6
      : SchemaTransformRule{
            "simple_properties_identifiers",
            "Set `properties` to identifier names that can be easily mapped to "
            "programming languages (matching [A-Za-z_][A-Za-z0-9_]*)"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &root,
            const sourcemeta::blaze::SchemaVocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &frame,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
        {SchemaVocabularies::Known::JSON_SCHEMA_2020_12_APPLICATOR,
         SchemaVocabularies::Known::JSON_SCHEMA_2019_09_APPLICATOR,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_4,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3_HYPER,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_2,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_2_HYPER,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1_HYPER}));
    ONLY_CONTINUE_IF(schema.is_object());
    const auto *properties{schema.try_at("properties")};
    ONLY_CONTINUE_IF(properties && properties->is_object() &&
                     !properties->empty());

    if (vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_SCHEMA_2020_12_CORE,
             SchemaVocabularies::Known::JSON_SCHEMA_2019_09_CORE})) {
      // Skip meta-schemas with `$vocabulary` (2019-09+)
      // We check the current schema resource (not root) to handle bundled
      // schemas
      const auto base_location{frame.traverse(location.base)};
      if (base_location.has_value()) {
        const auto &resource{get(root, base_location->get().pointer)};
        ONLY_CONTINUE_IF(!resource.is_object() ||
                         !resource.defines("$vocabulary"));
      }
    } else {
      // Skip pre-vocabulary meta-schemas
      JSON::String base_with_hash{location.base};
      base_with_hash += '#';
      ONLY_CONTINUE_IF(location.base != location.dialect &&
                       base_with_hash != location.dialect);
    }

    std::vector<Pointer> offenders;
    for (const auto &entry : properties->as_object()) {
      static const Regex IDENTIFIER_PATTERN{
          to_regex("^[A-Za-z_][A-Za-z0-9_]*$").value()};
      if (!matches(IDENTIFIER_PATTERN, entry.first)) {
        offenders.push_back(Pointer{"properties", entry.first});
      }
    }

    ONLY_CONTINUE_IF(!offenders.empty());
    return applies_to_pointers(std::move(offenders));
  }
};
