class UnknownKeywordsDrop final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  UnknownKeywordsDrop() : SchemaTransformRule{"unknown_keywords_drop"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &walker,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    // A keyword the dialect does not define says nothing about whether an
    // instance is valid, so the canonical form is the one without it. Every
    // other dialect keeps such a keyword under an `x-` prefix instead, which
    // `unknown_keywords_prefix` does, as their canonical forms describe a
    // nested document that has somewhere to put it
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3_HYPER}) &&
        schema.is_object());

    std::vector<sourcemeta::core::JSON::String> keywords;
    for (const auto &entry : schema.as_object()) {
      // If there is any i.e. optional vocabulary we don't recognise, then this
      // seemingly unknown keyword might belong to one of those, and thus it
      // might not be safe to flag it
      if (walker(entry.first, vocabularies).type ==
              SchemaKeywordType::Unknown &&
          !vocabularies.has_unknown()) {
        keywords.push_back(entry.first);
      }
    }

    ONLY_CONTINUE_IF(!keywords.empty());
    this->keywords_ = std::move(keywords);
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    for (const auto &keyword : this->keywords_) {
      schema.erase(keyword);
    }
  }

private:
  mutable std::vector<sourcemeta::core::JSON::String> keywords_;
};
