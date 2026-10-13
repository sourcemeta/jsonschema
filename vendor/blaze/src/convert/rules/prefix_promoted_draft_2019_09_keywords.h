class PrefixPromoted201909Keywords final : public SchemaTransformRule {
public:
  PrefixPromoted201909Keywords(
      std::vector<std::string_view> &&keywords_left_alone = {})
      : SchemaTransformRule{"prefix_promoted_2019_09_keywords"},
        keywords_left_alone_{std::move(keywords_left_alone)} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const Site &, const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains(SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7) &&
        schema.is_object());

    ONLY_CONTINUE_IF(
        promoted_keyword_is_author_data(schema, PROMOTING_DIALECT));

    for (const auto &keyword : KEYWORDS) {
      if (schema.defines(keyword) && !this->is_left_alone(schema, keyword)) {
        return true;
      }
    }

    return false;
  }

  auto transform(sourcemeta::core::JSON &schema, const Site &) const
      -> void override {
    this->renames_.clear();
    for (const auto &keyword : KEYWORDS) {
      const std::string keyword_name{keyword};
      if (!schema.defines(keyword_name) ||
          this->is_left_alone(schema, keyword)) {
        continue;
      }

      std::string prefixed_name{"x-" + keyword_name};
      while (schema.defines(prefixed_name)) {
        prefixed_name.insert(0, "x-");
      }

      this->renames_.emplace(keyword_name, prefixed_name);
      schema.rename(keyword_name, std::move(prefixed_name));
    }
  }

  [[nodiscard]] auto relocations() const -> std::vector<Relocation> override {
    std::vector<Relocation> result;
    result.reserve(this->renames_.size());
    for (const auto &[old_name, new_name] : this->renames_) {
      result.emplace_back(sourcemeta::core::Pointer{old_name},
                          sourcemeta::core::Pointer{new_name});
    }

    return result;
  }

private:
  static constexpr std::string_view PROMOTING_DIALECT{
      "https://json-schema.org/draft/2019-09/schema"};

  // NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
  static inline const std::array<std::string_view, 13> KEYWORDS{
      {"$anchor", "$recursiveAnchor", "$recursiveRef", "$vocabulary", "$defs",
       "dependentSchemas", "dependentRequired", "unevaluatedItems",
       "unevaluatedProperties", "maxContains", "minContains", "contentSchema",
       "deprecated"}};

  // A keyword the caller asked to carry across as the keyword it already was,
  // rather than as the author data it would otherwise be read as.
  //
  // The caller names the dialect of a document, not of each position in it, and
  // a document may embed a resource written in another dialect whose members
  // are exactly the author data this rule exists to protect. What tells the two
  // apart is the value: the source dialect constrained the keyword it named, so
  // one carrying a shape that dialect never allowed was never that keyword
  [[nodiscard]] auto is_left_alone(const sourcemeta::core::JSON &schema,
                                   const std::string_view keyword) const
      -> bool {
    if (std::ranges::find(this->keywords_left_alone_, keyword) ==
        this->keywords_left_alone_.cend()) {
      return false;
    }

    // Only a boolean is the annotation this names. A value of any other shape
    // was never that keyword, whatever it is called, and carrying it across as
    // one would leave an output the target's own meta-schema rejects
    return schema.at(sourcemeta::core::JSON::String{keyword}).is_boolean();
  }

  const std::vector<std::string_view> keywords_left_alone_;
  mutable std::unordered_map<std::string, std::string> renames_;
};
