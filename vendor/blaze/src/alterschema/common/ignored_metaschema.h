class IgnoredMetaschema final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  IgnoredMetaschema()
      : SchemaTransformRule{
            "ignored_metaschema",
            "A `$schema` declaration without a sibling identifier (or with a "
            "sibling `$ref` in Draft 7 and older dialects), is ignored"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(schema.is_object());
    const auto *schema_keyword{schema.try_at("$schema")};
    ONLY_CONTINUE_IF(schema_keyword && schema_keyword->is_string());
    const auto dialect{sourcemeta::blaze::dialect(schema)};
    ONLY_CONTINUE_IF(!dialect.empty());
    ONLY_CONTINUE_IF(dialect != location.dialect);
    return APPLIES_TO_KEYWORDS("$schema");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("$schema");
  }
};
