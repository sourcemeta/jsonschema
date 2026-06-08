class RecursiveAnchorFalseDrop final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  RecursiveAnchorFalseDrop()
      : SchemaTransformRule{"recursive_anchor_false_drop", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains(Vocabularies::Known::JSON_Schema_2019_09_Core) &&
        schema.is_object());

    const auto *recursive_anchor{schema.try_at("$recursiveAnchor")};
    ONLY_CONTINUE_IF(recursive_anchor && recursive_anchor->is_boolean() &&
                     !recursive_anchor->to_boolean());
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("$recursiveAnchor");
  }
};
