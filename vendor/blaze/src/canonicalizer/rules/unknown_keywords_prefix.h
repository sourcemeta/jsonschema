class UnknownKeywordsPrefix final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  UnknownKeywordsPrefix() : SchemaTransformRule{"unknown_keywords_prefix"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &, const Vocabularies &vocabularies,
            const SchemaFrame &, const SchemaFrame::Location &,
            const SchemaWalker &walker, const SchemaResolver &) const
      -> bool override {
    ONLY_CONTINUE_IF(schema.is_object());
    std::vector<sourcemeta::core::Pointer> locations;
    for (const auto &entry : schema.as_object()) {
      if (entry.first.starts_with("x-")) {
        continue;
      }

      const auto &metadata = walker(entry.first, vocabularies);
      if (metadata.type == SchemaKeywordType::Unknown &&
          // If there is any i.e. optional vocabulary we don't recognise, then
          // this seemingly unknown keyword might belong to one of those, and
          // thus it might not be safe to flag it
          !vocabularies.has_unknown()) {
        locations.push_back(sourcemeta::core::Pointer{entry.first});
      }
    }

    ONLY_CONTINUE_IF(!locations.empty());
    this->locations_ = std::move(locations);
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    this->renames_.clear();
    for (const auto &location : this->locations_) {
      const auto &keyword{location.at(0).to_property()};
      assert(schema.defines(keyword));
      std::string prefixed_name = "x-" + keyword;
      while (schema.defines(prefixed_name)) {
        prefixed_name.insert(0, "x-");
      }

      this->renames_.emplace(keyword, prefixed_name);
      schema.rename(keyword, std::move(prefixed_name));
    }
  }

  [[nodiscard]] auto rereference(const std::string_view,
                                 const sourcemeta::core::Pointer &,
                                 const sourcemeta::core::Pointer &target,
                                 const sourcemeta::core::Pointer &current) const
      -> std::optional<sourcemeta::core::Pointer> override {
    for (const auto &[old_name, new_name] : this->renames_) {
      auto result{
          target.rebase(current.concat(sourcemeta::core::Pointer{old_name}),
                        current.concat(sourcemeta::core::Pointer{new_name}))};
      if (result != target) {
        return result;
      }
    }

    return target;
  }

private:
  mutable std::unordered_map<std::string, std::string> renames_;

private:
  mutable std::vector<sourcemeta::core::Pointer> locations_;
};
