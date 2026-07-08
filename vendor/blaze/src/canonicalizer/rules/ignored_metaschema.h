class IgnoredMetaschema final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  IgnoredMetaschema() : SchemaTransformRule{"ignored_metaschema"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(schema.is_object());
    const auto *schema_keyword{schema.try_at("$schema")};
    ONLY_CONTINUE_IF(schema_keyword && schema_keyword->is_string());
    const auto dialect{sourcemeta::blaze::dialect(schema)};
    ONLY_CONTINUE_IF(!dialect.empty());
    ONLY_CONTINUE_IF(dialect != location.dialect);
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.erase("$schema");
  }
};
