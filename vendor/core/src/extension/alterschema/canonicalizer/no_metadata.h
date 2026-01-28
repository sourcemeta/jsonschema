class NoMetadata final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  NoMetadata()
      : SchemaTransformRule{"no_metadata",
                            "Annotations, comments, and unknown keywords have "
                            "no effect on validation"} {};

  [[nodiscard]] auto
  condition(const JSON &schema, const JSON &, const Vocabularies &vocabularies,
            const SchemaFrame &, const SchemaFrame::Location &,
            const SchemaWalker &walker, const SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(schema.is_object() && !schema.empty());

    std::vector<Pointer> locations;
    for (const auto &entry : schema.as_object()) {
      const auto &metadata{walker(entry.first, vocabularies)};
      if (metadata.type == SchemaKeywordType::Annotation ||
          metadata.type == SchemaKeywordType::Comment ||
          metadata.type == SchemaKeywordType::Unknown) {
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
