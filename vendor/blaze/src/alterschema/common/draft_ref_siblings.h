class DraftRefSiblings final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  DraftRefSiblings()
      : SchemaTransformRule{"draft_ref_siblings",
                            "In Draft 7 and older dialects, keywords sibling "
                            "to `$ref` are never evaluated"} {}

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &walker,
            const sourcemeta::blaze::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
        {Vocabularies::Known::JSON_Schema_Draft_7,
         Vocabularies::Known::JSON_Schema_Draft_6,
         Vocabularies::Known::JSON_Schema_Draft_4,
         Vocabularies::Known::JSON_Schema_Draft_3,
         Vocabularies::Known::JSON_Schema_Draft_3_Hyper,
         Vocabularies::Known::JSON_Schema_Draft_2,
         Vocabularies::Known::JSON_Schema_Draft_1,
         Vocabularies::Known::JSON_Schema_Draft_0}));
    ONLY_CONTINUE_IF(schema.is_object() && schema.defines("$ref"));

    std::vector<Pointer> locations;
    for (const auto &entry : schema.as_object()) {
      const auto &metadata{walker(entry.first, vocabularies)};
      if (metadata.type == sourcemeta::blaze::SchemaKeywordType::Reference ||
          metadata.type == sourcemeta::blaze::SchemaKeywordType::Comment ||
          // If we disallow this, we end up deleting it and the linter will fail
          // with an error about not knowing the dialect
          entry.first == "$schema") {
        continue;
      } else {
        locations.push_back(Pointer{entry.first});
      }
    }

    ONLY_CONTINUE_IF(!locations.empty());
    return APPLIES_TO_POINTERS(std::move(locations));
  }

  auto transform(JSON &schema, const Result &result) const -> void override {
    for (const auto &location : result.locations) {
      schema.erase(location.at(0).to_property());
    }
  }
};
