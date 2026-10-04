class PrefixPromoted202012Keywords final : public SchemaTransformRule {
public:
  PrefixPromoted202012Keywords()
      : SchemaTransformRule{"prefix_promoted_2020_12_keywords"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const Site &, const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(vocabularies.contains(
                         SchemaVocabularies::Known::JSON_SCHEMA_2019_09_CORE) &&
                     schema.is_object());

    ONLY_CONTINUE_IF(
        promoted_keyword_is_author_data(schema, PROMOTING_DIALECT));

    return schema.defines_any({"prefixItems", "$dynamicAnchor", "$dynamicRef"});
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
  static constexpr std::string_view PROMOTING_DIALECT{
      "https://json-schema.org/draft/2020-12/schema"};

  // NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
  static inline const std::array<std::string_view, 3> KEYWORDS{
      {"prefixItems", "$dynamicAnchor", "$dynamicRef"}};

  mutable std::unordered_map<std::string, std::string> renames_;
};
