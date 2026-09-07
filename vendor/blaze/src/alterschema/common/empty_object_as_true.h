class EmptyObjectAsTrue final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::false_type;
  EmptyObjectAsTrue()
      : SchemaTransformRule{
            "empty_object_as_true",
            "The empty schema `{}` accepts all values and is equivalent to the "
            "boolean schema `true`"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::SchemaVocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {SchemaVocabularies::Known::JSON_SCHEMA_2020_12_CORE,
                          SchemaVocabularies::Known::JSON_SCHEMA_2019_09_CORE,
                          SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7,
                          SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6,
                          SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_4}) &&
                     schema.is_object() && schema.empty());
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.into(JSON{true});
  }
};
