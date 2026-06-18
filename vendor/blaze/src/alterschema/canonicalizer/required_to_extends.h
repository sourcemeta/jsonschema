class RequiredToExtends final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  RequiredToExtends()
      : SchemaTransformRule{
            "required_to_extends",
            "In Draft 3 canonical form, `required` is only ever a sibling of "
            "`extends`; its other siblings are wrapped into an `extends` "
            "branch"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_Draft_3,
                          Vocabularies::Known::JSON_Schema_Draft_3_Hyper}) &&
                     schema.is_object());

    const auto *required{schema.try_at("required")};
    ONLY_CONTINUE_IF(required && required->is_boolean());

    for (const auto &entry : schema.as_object()) {
      if (!stays_at_top(entry.first)) {
        return true;
      }
    }

    return false;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    this->wrapped_keywords_.clear();
    for (const auto &entry : schema.as_object()) {
      if (!stays_at_top(entry.first)) {
        this->wrapped_keywords_.push_back(entry.first);
      }
    }

    auto branch{JSON::make_object()};
    for (const auto &keyword : this->wrapped_keywords_) {
      branch.assign(keyword, schema.at(keyword));
    }

    for (const auto &keyword : this->wrapped_keywords_) {
      schema.erase(keyword);
    }

    if (schema.defines("extends") && schema.at("extends").is_array()) {
      this->branch_index_ = schema.at("extends").size();
      schema.at("extends").push_back(std::move(branch));
    } else if (schema.defines("extends")) {
      // Draft 3 allows `extends` to be a single schema; preserve it as the
      // first branch of the new array
      auto extends{JSON::make_array()};
      extends.push_back(schema.at("extends"));
      this->branch_index_ = extends.size();
      extends.push_back(std::move(branch));
      schema.assign("extends", std::move(extends));
    } else {
      this->branch_index_ = 0;
      auto extends{JSON::make_array()};
      extends.push_back(std::move(branch));
      schema.assign("extends", std::move(extends));
    }
  }

  [[nodiscard]] auto rereference(const std::string_view, const Pointer &,
                                 const Pointer &target,
                                 const Pointer &current) const
      -> Pointer override {
    for (const auto &keyword : this->wrapped_keywords_) {
      const auto keyword_prefix{current.concat({keyword})};
      if (target.starts_with(keyword_prefix)) {
        return target.rebase(
            keyword_prefix,
            current.concat({"extends", this->branch_index_, keyword}));
      }
    }

    return target;
  }

private:
  static auto stays_at_top(const sourcemeta::core::JSON::String &keyword)
      -> bool {
    return keyword == "required" || keyword == "extends" ||
           keyword == "$schema" || keyword == "id" || keyword == "$ref";
  }

  mutable std::vector<sourcemeta::core::JSON::String> wrapped_keywords_;
  mutable std::size_t branch_index_{0};
};
