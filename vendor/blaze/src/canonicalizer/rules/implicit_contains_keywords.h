class ImplicitContainsKeywords final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  ImplicitContainsKeywords()
      : SchemaTransformRule{"implicit_contains_keywords"} {};

  [[nodiscard]] auto condition(
      const sourcemeta::core::JSON &schema, const sourcemeta::core::JSON &root,
      const sourcemeta::core::SchemaVocabularies &vocabularies,
      const sourcemeta::core::SchemaFrame &frame,
      const sourcemeta::core::SchemaFrame::Location &location,
      const sourcemeta::core::SchemaWalker &walker,
      const sourcemeta::core::SchemaResolver &resolver) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_SCHEMA_2019_09_APPLICATOR,
             SchemaVocabularies::Known::JSON_SCHEMA_2020_12_APPLICATOR}) &&
        schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type && type->is_string() && type->to_string() == "array");

    if (schema.defines("contains")) {
      ONLY_CONTINUE_IF(!schema.defines("minContains"));
    } else {
      ONLY_CONTINUE_IF(!schema.defines("minContains") &&
                       !schema.defines("maxContains"));
      ONLY_CONTINUE_IF(
          !walk_up_in_place_applicators(
               root, frame, location, walker, resolver,
               [](const sourcemeta::core::JSON &ancestor,
                  const SchemaVocabularies &ancestor_vocabularies) -> bool {
                 return ancestor.defines("unevaluatedItems") &&
                        ancestor_vocabularies.contains(
                            SchemaVocabularies::Known::
                                JSON_SCHEMA_2020_12_UNEVALUATED);
               })
               .has_value());
    }

    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    if (!schema.defines("contains")) {
      schema.assign("contains", sourcemeta::core::JSON{true});
      schema.assign("minContains", sourcemeta::core::JSON{0});
    } else {
      schema.assign("minContains", sourcemeta::core::JSON{1});
    }
  }
};
