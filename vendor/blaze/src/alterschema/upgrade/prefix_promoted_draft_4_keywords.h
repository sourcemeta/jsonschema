class PrefixPromotedDraft4Keywords final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  PrefixPromotedDraft4Keywords()
      : SchemaTransformRule{"prefix_promoted_draft_4_keywords", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains(Vocabularies::Known::JSON_Schema_Draft_3) &&
        schema.is_object());

    for (const auto &keyword : KEYWORDS) {
      if (schema.defines(keyword)) {
        return true;
      }
    }

    return false;
  }

  auto transform(sourcemeta::core::JSON &schema, const Result &) const
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

  [[nodiscard]] auto rereference(const std::string_view,
                                 const sourcemeta::core::Pointer &,
                                 const sourcemeta::core::Pointer &target,
                                 const sourcemeta::core::Pointer &current) const
      -> sourcemeta::core::Pointer override {
    for (const auto &[old_name, new_name] : this->renames_) {
      const auto result{
          target.rebase(current.concat(sourcemeta::core::Pointer{old_name}),
                        current.concat(sourcemeta::core::Pointer{new_name}))};
      if (result != target) {
        return result;
      }
    }

    return target;
  }

private:
  // NOLINTNEXTLINE(bugprone-throwing-static-initialization)
  static inline const std::array<std::string_view, 7> KEYWORDS{
      {"multipleOf", "maxProperties", "minProperties", "allOf", "anyOf",
       "oneOf", "not"}};

  mutable std::unordered_map<std::string, std::string> renames_;
};
