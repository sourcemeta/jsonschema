class ContentSchemaDefault final : public SchemaTransformRule {
public:
  ContentSchemaDefault()
      : SchemaTransformRule{
            "content_schema_default",
            "Setting the `contentSchema` keyword to the true schema "
            "does not add any further constraint"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Content,
                          Vocabularies::Known::JSON_Schema_2019_09_Content}) &&
                     schema.is_object() && schema.defines("contentSchema") &&
                     ((schema.at("contentSchema").is_boolean() &&
                       schema.at("contentSchema").to_boolean()) ||
                      (schema.at("contentSchema").is_object() &&
                       schema.at("contentSchema").empty())));
    ONLY_CONTINUE_IF(!frame.has_references_through(
        location.pointer.concat({"contentSchema"})));
    return APPLIES_TO_KEYWORDS("contentSchema");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("contentSchema");
  }
};
