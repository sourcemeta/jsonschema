class DefinitionsToDefs final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  DefinitionsToDefs()
      : SchemaTransformRule{"definitions_to_defs",
                            "`definitions` was superseded by `$defs` in "
                            "2019-09 and later versions"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::SchemaVocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_SCHEMA_2020_12_CORE,
             SchemaVocabularies::Known::JSON_SCHEMA_2019_09_CORE}) &&
        schema.is_object() && schema.defines("definitions") &&
        !schema.defines("$defs"));
    return applies_to_keywords("definitions");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.rename("definitions", "$defs");
  }

  [[nodiscard]] auto rereference(const std::string_view, const Pointer &,
                                 const Pointer &target,
                                 const Pointer &current) const
      -> Pointer override {
    return target.rebase(current.concat("definitions"),
                         current.concat("$defs"));
  }
};
