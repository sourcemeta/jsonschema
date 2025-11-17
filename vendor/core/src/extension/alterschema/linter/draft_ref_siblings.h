class DraftRefSiblings final : public SchemaTransformRule {
public:
  DraftRefSiblings()
      : SchemaTransformRule{"draft_ref_siblings",
                            "In Draft 7 and older dialects, keywords sibling "
                            "to `$ref` are never evaluated"} {}

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &walker,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(contains_any(vocabularies,
                                  {"http://json-schema.org/draft-07/schema#",
                                   "http://json-schema.org/draft-06/schema#",
                                   "http://json-schema.org/draft-04/schema#",
                                   "http://json-schema.org/draft-03/schema#",
                                   "http://json-schema.org/draft-02/schema#",
                                   "http://json-schema.org/draft-01/schema#",
                                   "http://json-schema.org/draft-00/schema#"}));
    ONLY_CONTINUE_IF(schema.is_object() && schema.defines("$ref"));

    std::vector<Pointer> locations;
    for (const auto &entry : schema.as_object()) {
      const auto metadata{walker(entry.first, vocabularies)};
      if (metadata.type == sourcemeta::core::SchemaKeywordType::Reference ||
          metadata.type == sourcemeta::core::SchemaKeywordType::Comment ||
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
