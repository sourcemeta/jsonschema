class MetadataKeywordsDrop final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  MetadataKeywordsDrop() : SchemaTransformRule{"metadata_keywords_drop"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    // The canonical form says what makes an instance valid, and none of these
    // takes part in that: they name a schema, describe it, and say what to
    // put where an instance says nothing. Two documents that differ in none
    // but these accept the same instances, so the canonical form is the one
    // without them.
    //
    // These are the metadata keywords of the Draft 3 meta-schema, named here
    // rather than asked of the walker: a document on a pre-vocabulary dialect
    // puts every keyword it knows in the one vocabulary that dialect stands
    // for, `title` and `type` alike
    //
    // Only Draft 3 is taken this far. The canonical form of every other
    // dialect still describes a nested document, which leaves these keywords
    // somewhere to sit, so dropping them there would be a change of its own
    static const std::set<sourcemeta::core::JSON::String> METADATA_KEYWORDS{
        "default", "description", "title"};

    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3_HYPER}) &&
        schema.is_object());

    std::vector<sourcemeta::core::JSON::String> keywords;
    for (const auto &entry : schema.as_object()) {
      if (METADATA_KEYWORDS.contains(entry.first)) {
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
