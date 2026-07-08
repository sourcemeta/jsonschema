class ItemsImplicit final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  ItemsImplicit() : SchemaTransformRule{"items_implicit"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &root,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &frame,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &walker,
            const sourcemeta::blaze::SchemaResolver &resolver) const
      -> bool override {
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
        schema.is_object() && !schema.defines("items"));

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type && type->is_string() && type->to_string() == "array");
    ONLY_CONTINUE_IF(
        !(schema.defines("unevaluatedItems") &&
          vocabularies.contains_any(
              {Vocabularies::Known::JSON_Schema_2020_12_Unevaluated,
               Vocabularies::Known::JSON_Schema_2019_09_Applicator})));
    ONLY_CONTINUE_IF(
        !WALK_UP_IN_PLACE_APPLICATORS(
             root, frame, location, walker, resolver,
             [](const sourcemeta::core::JSON &ancestor,
                const Vocabularies &ancestor_vocabularies) -> bool {
               return ancestor.defines("unevaluatedItems") &&
                      ancestor_vocabularies.contains_any(
                          {Vocabularies::Known::JSON_Schema_2020_12_Unevaluated,
                           Vocabularies::Known::
                               JSON_Schema_2019_09_Applicator});
             })
             .has_value());
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.assign("items", sourcemeta::core::JSON{true});
  }
};
