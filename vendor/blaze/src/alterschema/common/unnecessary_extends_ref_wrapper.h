class UnnecessaryExtendsRefWrapper final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  UnnecessaryExtendsRefWrapper()
      : SchemaTransformRule{"unnecessary_extends_ref_wrapper",
                            "Wrapping `$ref` in `extends` is only necessary if "
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
    ONLY_CONTINUE_IF(vocabularies.contains_any(
        {Vocabularies::Known::JSON_Schema_Draft_3,
         Vocabularies::Known::JSON_Schema_Draft_3_Hyper}));
    ONLY_CONTINUE_IF(schema.is_object() && schema.size() == 1);
    const auto *extends{schema.try_at("extends")};
    ONLY_CONTINUE_IF(extends);

    // In Draft 3, `$ref` overrides sibling keywords, so we can only elevate
    // it if it is the only keyword of the only branch, and the outer
    // subschema only declares `extends`
    if (extends->is_object()) {
      ONLY_CONTINUE_IF(extends->size() == 1 && extends->defines("$ref"));
      return APPLIES_TO_POINTERS({{"extends", "$ref"}});
    }

    if (extends->is_array()) {
      ONLY_CONTINUE_IF(extends->size() == 1);
      const auto &branch{extends->at(0)};
      ONLY_CONTINUE_IF(branch.is_object());
      ONLY_CONTINUE_IF(branch.size() == 1 && branch.defines("$ref"));
      return APPLIES_TO_POINTERS({{"extends", 0, "$ref"}});
    }

    return false;
  }

  auto transform(JSON &schema, const Result &result) const -> void override {
    const auto &location{result.locations.at(0)};
    if (location.size() == 3) {
      auto value{schema.at("extends").at(0).at("$ref")};
      schema.at("extends").into(std::move(value));
    } else {
      auto value{schema.at("extends").at("$ref")};
      schema.at("extends").into(std::move(value));
    }
    schema.rename("extends", "$ref");
  }
};
