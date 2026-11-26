class DraftRefSiblings final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  DraftRefSiblings() : SchemaTransformRule{"draft_ref_siblings"} {}

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &walker,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(reference_overrides_siblings(vocabularies));
    ONLY_CONTINUE_IF(schema.is_object() && schema.defines("$ref"));

    std::vector<sourcemeta::core::Pointer> locations;
    for (const auto &entry : schema.as_object()) {
      const auto &metadata{walker(entry.first, vocabularies)};
      if (metadata.type == sourcemeta::core::SchemaKeywordType::Reference ||
          metadata.type == sourcemeta::core::SchemaKeywordType::Comment) {
        continue;
      }
      locations.push_back(sourcemeta::core::Pointer{entry.first});
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
