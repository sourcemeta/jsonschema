class ItemsImplicit final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  ItemsImplicit() : SchemaTransformRule{"items_implicit"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &root,
            const sourcemeta::blaze::SchemaVocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &frame,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &walker,
            const sourcemeta::blaze::SchemaResolver &resolver) const
      -> bool override {
    ONLY_CONTINUE_IF(
        ((vocabularies.contains(
              SchemaVocabularies::Known::JSON_SCHEMA_2020_12_VALIDATION) &&
          vocabularies.contains(
              SchemaVocabularies::Known::JSON_SCHEMA_2020_12_APPLICATOR)) ||
         (vocabularies.contains(
              SchemaVocabularies::Known::JSON_SCHEMA_2019_09_VALIDATION) &&
          vocabularies.contains(
              SchemaVocabularies::Known::JSON_SCHEMA_2019_09_APPLICATOR)) ||
         vocabularies.contains_any(
             {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7,
              SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6})) &&
        schema.is_object() && !schema.defines("items"));

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type && type->is_string() && type->to_string() == "array");
    ONLY_CONTINUE_IF(
        !(schema.defines("unevaluatedItems") &&
          vocabularies.contains_any(
              {SchemaVocabularies::Known::JSON_SCHEMA_2020_12_UNEVALUATED,
               SchemaVocabularies::Known::JSON_SCHEMA_2019_09_APPLICATOR})));
    ONLY_CONTINUE_IF(
        !walk_up_in_place_applicators(
             root, frame, location, walker, resolver,
             [](const sourcemeta::core::JSON &ancestor,
                const SchemaVocabularies &ancestor_vocabularies) -> bool {
               return ancestor.defines("unevaluatedItems") &&
                      ancestor_vocabularies.contains_any(
                          {SchemaVocabularies::Known::
                               JSON_SCHEMA_2020_12_UNEVALUATED,
                           SchemaVocabularies::Known::
                               JSON_SCHEMA_2019_09_APPLICATOR});
             })
             .has_value());
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.assign("items", sourcemeta::core::JSON{true});
  }
};
