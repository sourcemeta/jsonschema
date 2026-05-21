class EmptyDefinitionsDrop final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  EmptyDefinitionsDrop() : SchemaTransformRule{"empty_definitions_drop", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any({Vocabularies::Known::JSON_Schema_Draft_4,
                                   Vocabularies::Known::JSON_Schema_Draft_6,
                                   Vocabularies::Known::JSON_Schema_Draft_7}) &&
        schema.is_object());

    const auto *definitions{schema.try_at("definitions")};
    ONLY_CONTINUE_IF(definitions && definitions->is_object() &&
                     definitions->empty());
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("definitions");
  }
};
