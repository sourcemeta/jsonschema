class DefinitionsToDefs final : public SchemaTransformRule {
public:
  DefinitionsToDefs() : SchemaTransformRule{"definitions_to_defs"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const Site &, const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_SCHEMA_2020_12_CORE,
             SchemaVocabularies::Known::JSON_SCHEMA_2019_09_CORE}) &&
        schema.is_object() && schema.defines("definitions") &&
        !schema.defines("$defs"));
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema, const Site &) const
      -> void override {
    schema.rename("definitions", "$defs");
  }

  [[nodiscard]] auto relocations() const -> std::vector<Relocation> override {
    return {{sourcemeta::core::Pointer{"definitions"},
             sourcemeta::core::Pointer{"$defs"}}};
  }
};
