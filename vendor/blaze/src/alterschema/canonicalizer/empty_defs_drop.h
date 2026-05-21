class EmptyDefsDrop final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  EmptyDefsDrop() : SchemaTransformRule{"empty_defs_drop", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2019_09_Core,
                          Vocabularies::Known::JSON_Schema_2020_12_Core}) &&
                     schema.is_object());

    const auto *defs{schema.try_at("$defs")};
    ONLY_CONTINUE_IF(defs && defs->is_object() && defs->empty());
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("$defs");
  }
};
