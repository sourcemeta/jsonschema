class IfThenElseImplicit final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  IfThenElseImplicit() : SchemaTransformRule{"if_then_else_implicit", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_Draft_7,
             Vocabularies::Known::JSON_Schema_2019_09_Applicator,
             Vocabularies::Known::JSON_Schema_2020_12_Applicator}) &&
        schema.is_object() && schema.defines("if") &&
        (schema.defines("then") || schema.defines("else")) &&
        (!schema.defines("then") || !schema.defines("else")));
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    if (!schema.defines("then")) {
      schema.assign("then", JSON{true});
    }
    if (!schema.defines("else")) {
      schema.assign("else", JSON{true});
    }
  }
};
