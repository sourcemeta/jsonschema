class PrefixPromotedOpenAPI31Keywords final : public SchemaTransformRule {
public:
  PrefixPromotedOpenAPI31Keywords(
      std::vector<std::string_view> &&keywords_left_alone = {})
      : SchemaTransformRule{"prefix_promoted_openapi_3_1_keywords"},
        keywords_left_alone_{std::move(keywords_left_alone)} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const Site &, const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    // The OpenAPI dialect requires every 2020-12 vocabulary, so the core
    // vocabulary alone cannot tell the two rungs apart. Without the second
    // half, a subschema of a document already on the OpenAPI dialect declares
    // no dialect of its own, reads as sitting below the rung, and has the
    // keyword that is doing its job renamed out from under it
    ONLY_CONTINUE_IF(vocabularies.contains(
                         SchemaVocabularies::Known::JSON_SCHEMA_2020_12_CORE) &&
                     !vocabularies.contains_any(
                         {SchemaVocabularies::Known::OPENAPI_3_1_BASE,
                          SchemaVocabularies::Known::OPENAPI_3_2_BASE}) &&
                     schema.is_object());

    ONLY_CONTINUE_IF(
        promoted_keyword_is_author_data(schema, OPENAPI_3_1_DIALECT));

    for (const auto &keyword : KEYWORDS) {
      if (schema.defines(sourcemeta::core::JSON::String{keyword}) &&
          !this->is_left_alone(schema, keyword)) {
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
  // NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
  static inline const std::array<std::string_view, 4> KEYWORDS{
      {"example", "discriminator", "externalDocs", "xml"}};

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

    // Three of these name an object and `example` names anything at all. A
    // value of another shape was never the keyword it is named after, and
    // carrying it across as one would leave an output the target's own
    // meta-schema rejects
    return keyword == "example" ||
           schema.at(sourcemeta::core::JSON::String{keyword}).is_object();
  }

  const std::vector<std::string_view> keywords_left_alone_;
  mutable std::unordered_map<std::string, std::string> renames_;
};
