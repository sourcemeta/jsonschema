class ItemsImplicit final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  ItemsImplicit()
      : SchemaTransformRule{"items_implicit",
                            "Every array has an implicit `items` "
                            "that consists of the boolean schema `true`"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &root,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &walker,
            const sourcemeta::core::SchemaResolver &resolver) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        ((vocabularies.contains(
              Vocabularies::Known::JSON_Schema_2020_12_Validation) &&
          vocabularies.contains(
              Vocabularies::Known::JSON_Schema_2020_12_Applicator)) ||
         (vocabularies.contains(
              Vocabularies::Known::JSON_Schema_2019_09_Validation) &&
          vocabularies.contains(
              Vocabularies::Known::JSON_Schema_2019_09_Applicator)) ||
         vocabularies.contains_any(
             {Vocabularies::Known::JSON_Schema_Draft_7,
              Vocabularies::Known::JSON_Schema_Draft_6})) &&
        schema.is_object() && schema.defines("type") &&
        schema.at("type").is_string() &&
        schema.at("type").to_string() == "array" && !schema.defines("items"));
    ONLY_CONTINUE_IF(
        !(schema.defines("unevaluatedItems") &&
          vocabularies.contains_any(
              {Vocabularies::Known::JSON_Schema_2020_12_Unevaluated,
               Vocabularies::Known::JSON_Schema_2019_09_Applicator})));
    ONLY_CONTINUE_IF(
        !WALK_UP_IN_PLACE_APPLICATORS(
             root, frame, location, walker, resolver,
             [](const JSON &ancestor,
                const Vocabularies &ancestor_vocabularies) {
               return ancestor.defines("unevaluatedItems") &&
                      ancestor_vocabularies.contains_any(
                          {Vocabularies::Known::JSON_Schema_2020_12_Unevaluated,
                           Vocabularies::Known::
                               JSON_Schema_2019_09_Applicator});
             })
             .has_value());
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.assign("items", JSON{true});
  }
};
