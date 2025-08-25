class DependenciesDefault final : public SchemaTransformRule {
public:
  DependenciesDefault()
      : SchemaTransformRule{
            "dependencies_default",
            "Setting the `dependencies` keyword to an empty object "
            "does not add any further constraint"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        contains_any(vocabularies,
                     {"http://json-schema.org/draft-07/schema#",
                      "http://json-schema.org/draft-06/schema#",
                      "http://json-schema.org/draft-04/schema#",
                      "http://json-schema.org/draft-03/schema#"}) &&
        schema.is_object() && schema.defines("dependencies") &&
        schema.at("dependencies").is_object() &&
        schema.at("dependencies").empty());
    return APPLIES_TO_KEYWORDS("dependencies");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("dependencies");
  }
};
