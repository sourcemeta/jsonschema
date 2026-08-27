class DraftRefSiblings final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  DraftRefSiblings() : SchemaTransformRule{"draft_ref_siblings"} {}

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::SchemaVocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &walker,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
        {SchemaVocabularies::Known::JSON_Schema_Draft_7,
         SchemaVocabularies::Known::JSON_Schema_Draft_6,
         SchemaVocabularies::Known::JSON_Schema_Draft_4,
         SchemaVocabularies::Known::JSON_Schema_Draft_3,
         SchemaVocabularies::Known::JSON_Schema_Draft_3_Hyper,
         SchemaVocabularies::Known::JSON_Schema_Draft_2,
         SchemaVocabularies::Known::JSON_Schema_Draft_1,
         SchemaVocabularies::Known::JSON_Schema_Draft_0}));
    ONLY_CONTINUE_IF(schema.is_object() && schema.defines("$ref"));

    std::vector<sourcemeta::core::Pointer> locations;
    for (const auto &entry : schema.as_object()) {
      const auto &metadata{walker(entry.first, vocabularies)};
      if (metadata.type == sourcemeta::blaze::SchemaKeywordType::Reference ||
          metadata.type == sourcemeta::blaze::SchemaKeywordType::Comment ||
          // If we disallow this, we end up deleting it and the linter will fail
          // with an error about not knowing the dialect
          entry.first == "$schema") {
        continue;
      } else {
        locations.push_back(sourcemeta::core::Pointer{entry.first});
      }
    }

    ONLY_CONTINUE_IF(!locations.empty());
    this->locations_ = std::move(locations);
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    for (const auto &location : this->locations_) {
      schema.erase(location.at(0).to_property());
    }
  }

private:
  mutable std::vector<sourcemeta::core::Pointer> locations_;
};
