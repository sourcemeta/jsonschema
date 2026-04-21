class DropAllOfEmptySchemas final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  DropAllOfEmptySchemas()
      : SchemaTransformRule{"drop_allof_empty_schemas",
                            "Empty schemas in `allOf` are redundant and can be "
                            "removed"} {};

  [[nodiscard]] auto
  condition(const JSON &schema, const JSON &, const Vocabularies &vocabularies,
            const SchemaFrame &, const SchemaFrame::Location &,
            const SchemaWalker &, const SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
        {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
         Vocabularies::Known::JSON_Schema_2019_09_Applicator,
         Vocabularies::Known::JSON_Schema_Draft_7,
         Vocabularies::Known::JSON_Schema_Draft_6,
         Vocabularies::Known::JSON_Schema_Draft_4}));
    ONLY_CONTINUE_IF(schema.is_object() && schema.defines("allOf") &&
                     schema.at("allOf").is_array() &&
                     !schema.at("allOf").empty());
    ONLY_CONTINUE_IF(
        std::ranges::any_of(schema.at("allOf").as_array(), is_empty_schema));
    return APPLIES_TO_KEYWORDS("allOf");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    auto new_allof{JSON::make_array()};
    for (const auto &entry : schema.at("allOf").as_array()) {
      if (!is_empty_schema(entry)) {
        new_allof.push_back(entry);
      }
    }

    if (new_allof.empty()) {
      schema.erase("allOf");
    } else {
      // Re-assign instead of the deleting in place to invalid memory addresses
      // and avoid confusing the transformer
      schema.assign("allOf", std::move(new_allof));
    }
  }
};
