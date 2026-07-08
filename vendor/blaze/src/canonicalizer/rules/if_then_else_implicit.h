class IfThenElseImplicit final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  IfThenElseImplicit() : SchemaTransformRule{"if_then_else_implicit"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
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

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    if (!schema.defines("then")) {
      schema.assign("then", sourcemeta::core::JSON{true});
    }
    if (!schema.defines("else")) {
      schema.assign("else", sourcemeta::core::JSON{true});
    }
  }
};
