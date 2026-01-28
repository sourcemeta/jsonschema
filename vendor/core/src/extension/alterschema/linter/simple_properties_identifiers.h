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
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
        {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
         Vocabularies::Known::JSON_Schema_2019_09_Applicator,
         Vocabularies::Known::JSON_Schema_Draft_7,
         Vocabularies::Known::JSON_Schema_Draft_6,
         Vocabularies::Known::JSON_Schema_Draft_4,
         Vocabularies::Known::JSON_Schema_Draft_3,
         Vocabularies::Known::JSON_Schema_Draft_2,
         Vocabularies::Known::JSON_Schema_Draft_2_Hyper,
         Vocabularies::Known::JSON_Schema_Draft_1,
         Vocabularies::Known::JSON_Schema_Draft_1_Hyper}));
    ONLY_CONTINUE_IF(schema.is_object() && schema.defines("properties") &&
                     schema.at("properties").is_object() &&
                     !schema.at("properties").empty());

    if (vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Core,
             Vocabularies::Known::JSON_Schema_2019_09_Core})) {
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
    for (const auto &entry : schema.at("properties").as_object()) {
      static const Regex IDENTIFIER_PATTERN{
          to_regex("^[A-Za-z_][A-Za-z0-9_]*$").value()};
      if (!matches(IDENTIFIER_PATTERN, entry.first)) {
        offenders.push_back(Pointer{"properties", entry.first});
      }
    }

    ONLY_CONTINUE_IF(!offenders.empty());
    return APPLIES_TO_POINTERS(std::move(offenders));
  }
};
