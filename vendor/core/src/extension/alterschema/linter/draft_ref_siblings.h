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
    if (!contains_any(vocabularies,
                      {"http://json-schema.org/draft-07/schema#",
                       "http://json-schema.org/draft-06/schema#",
                       "http://json-schema.org/draft-04/schema#",
                       "http://json-schema.org/draft-03/schema#",
                       "http://json-schema.org/draft-02/schema#",
                       "http://json-schema.org/draft-01/schema#",
                       "http://json-schema.org/draft-00/schema#"})) {
      return false;
    }

    if (!schema.is_object() || !schema.defines("$ref")) {
      return false;
    }

    // Clear and populate the preserve_keywords set
    this->preserve_keywords.clear();
    bool has_removable_siblings = false;

    // Loop over the object properties in schema
    for (const auto &entry : schema.as_object()) {
      const auto &key = entry.first;

      // Run the walker and only add to preserve_keywords the keywords whose
      // type is Other or Reference
      const auto metadata = walker(key, vocabularies);
      if (metadata.type == sourcemeta::core::SchemaKeywordType::Other ||
          metadata.type == sourcemeta::core::SchemaKeywordType::Reference) {
        this->preserve_keywords.insert(key);
      } else {
        has_removable_siblings = true;
      }
    }

    return has_removable_siblings;
  }

  auto transform(JSON &schema) const -> void override {
    schema.clear_except(this->preserve_keywords.cbegin(),
                        this->preserve_keywords.cend());
  }

private:
  mutable std::unordered_set<std::string> preserve_keywords;
};
