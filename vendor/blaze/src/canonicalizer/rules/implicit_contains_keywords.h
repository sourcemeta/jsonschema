class ImplicitContainsKeywords final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  ImplicitContainsKeywords()
      : SchemaTransformRule{"implicit_contains_keywords"} {};

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
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2019_09_Applicator,
             Vocabularies::Known::JSON_Schema_2020_12_Applicator}) &&
        schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type && type->is_string() && type->to_string() == "array");

    if (schema.defines("contains")) {
      ONLY_CONTINUE_IF(!schema.defines("minContains"));
    } else {
      ONLY_CONTINUE_IF(!schema.defines("minContains") &&
                       !schema.defines("maxContains"));
      ONLY_CONTINUE_IF(
          !WALK_UP_IN_PLACE_APPLICATORS(
               root, frame, location, walker, resolver,
               [](const sourcemeta::core::JSON &ancestor,
                  const Vocabularies &ancestor_vocabularies) -> bool {
                 return ancestor.defines("unevaluatedItems") &&
                        ancestor_vocabularies.contains(
                            Vocabularies::Known::
                                JSON_Schema_2020_12_Unevaluated);
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
