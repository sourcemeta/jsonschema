class IfThenElseImplicit final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  IfThenElseImplicit() : SchemaTransformRule{"if_then_else_implicit"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7,
             SchemaVocabularies::Known::JSON_SCHEMA_2019_09_APPLICATOR,
             SchemaVocabularies::Known::JSON_SCHEMA_2020_12_APPLICATOR}) &&
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
