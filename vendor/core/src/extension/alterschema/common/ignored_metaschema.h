class IgnoredMetaschema final : public SchemaTransformRule {
public:
  IgnoredMetaschema()
      : SchemaTransformRule{
            "ignored_metaschema",
            "A `$schema` declaration without a sibling identifier (or with a "
            "sibling `$ref` in Draft 7 and older dialects), is ignored"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(schema.is_object() && schema.defines("$schema") &&
                     schema.at("$schema").is_string());
    const auto dialect{sourcemeta::core::dialect(schema)};
    ONLY_CONTINUE_IF(dialect.has_value());
    ONLY_CONTINUE_IF(dialect.value() != location.dialect);
    return APPLIES_TO_KEYWORDS("$schema");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("$schema");
  }
};
