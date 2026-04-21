class RecursiveAnchorFalseDrop final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  RecursiveAnchorFalseDrop()
      : SchemaTransformRule{"recursive_anchor_false_drop", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains(Vocabularies::Known::JSON_Schema_2019_09_Core) &&
        schema.is_object() && schema.defines("$recursiveAnchor") &&
        schema.at("$recursiveAnchor").is_boolean() &&
        !schema.at("$recursiveAnchor").to_boolean());
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("$recursiveAnchor");
  }
};
