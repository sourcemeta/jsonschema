class IgnoredMetaschema final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  IgnoredMetaschema() : SchemaTransformRule{"ignored_metaschema"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(schema.is_object());
    const auto *schema_keyword{schema.try_at("$schema")};
    ONLY_CONTINUE_IF(schema_keyword && schema_keyword->is_string());
    const auto dialect{declared_dialect(schema)};
    ONLY_CONTINUE_IF(!dialect.empty());
    ONLY_CONTINUE_IF(dialect != location.dialect);
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.erase("$schema");
  }
};
