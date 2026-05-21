class UnnecessaryAllOfRefWrapperDraft final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  UnnecessaryAllOfRefWrapperDraft()
      : SchemaTransformRule{"unnecessary_allof_ref_wrapper_draft",
                            "Wrapping `$ref` in `allOf` is only necessary if "
                            "there are other sibling keywords"} {};

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
        vocabularies.contains_any({Vocabularies::Known::JSON_Schema_Draft_7,
                                   Vocabularies::Known::JSON_Schema_Draft_6,
                                   Vocabularies::Known::JSON_Schema_Draft_4}));
    ONLY_CONTINUE_IF(schema.is_object() && schema.size() == 1);
    const auto *all_of{schema.try_at("allOf")};
    ONLY_CONTINUE_IF(all_of && all_of->is_array());

    // In Draft 7 and older, `$ref` overrides sibling keywords, so we can only
    // elevate it if it is the only keyword of the only branch, and the outer
    // subschema only declares `allOf`
    ONLY_CONTINUE_IF(all_of->size() == 1);
    const auto &entry{all_of->at(0)};
    ONLY_CONTINUE_IF(entry.is_object());
    ONLY_CONTINUE_IF(entry.size() == 1 && entry.defines("$ref"));

    return APPLIES_TO_POINTERS({{"allOf", 0, "$ref"}});
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    auto value{schema.at("allOf").at(0).at("$ref")};
    schema.at("allOf").into(std::move(value));
    schema.rename("allOf", "$ref");
  }
};
