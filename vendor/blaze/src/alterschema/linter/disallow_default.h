class DisallowDefault final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  DisallowDefault()
      : SchemaTransformRule{"disallow_default",
                            "Setting the `disallow` keyword to the empty "
                            "array does not add any further constraint"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_Draft_3,
                          Vocabularies::Known::JSON_Schema_Draft_3_Hyper}) &&
                     schema.is_object());

    const auto *disallow{schema.try_at("disallow")};
    ONLY_CONTINUE_IF(disallow && disallow->is_array() && disallow->empty());
    return APPLIES_TO_KEYWORDS("disallow");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("disallow");
  }
};
