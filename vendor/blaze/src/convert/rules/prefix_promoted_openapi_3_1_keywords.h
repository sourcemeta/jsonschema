class PrefixPromotedOpenAPI31Keywords final : public SchemaTransformRule {
public:
  PrefixPromotedOpenAPI31Keywords()
      : SchemaTransformRule{"prefix_promoted_openapi_3_1_keywords"} {};

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

    return schema.defines_any(
        {"example", "discriminator", "externalDocs", "xml"});
  }

  auto transform(sourcemeta::core::JSON &schema, const Site &) const
      -> void override {
    this->renames_.clear();
    for (const auto &keyword : KEYWORDS) {
      const std::string keyword_name{keyword};
      if (!schema.defines(keyword_name)) {
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

  mutable std::unordered_map<std::string, std::string> renames_;
};
