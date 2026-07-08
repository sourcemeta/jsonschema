class DisallowArrayToExtends final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  DisallowArrayToExtends()
      : SchemaTransformRule{"disallow_array_to_extends"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_Draft_3,
                          Vocabularies::Known::JSON_Schema_Draft_3_Hyper}) &&
                     schema.is_object());

    const auto *disallow{schema.try_at("disallow")};
    ONLY_CONTINUE_IF(disallow && disallow->is_array() && disallow->size() > 1);
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    auto branches{sourcemeta::core::JSON::make_array()};
    for (const auto &element : schema.at("disallow").as_array()) {
      auto negation{sourcemeta::core::JSON::make_array()};
      negation.push_back(element);
      auto branch{sourcemeta::core::JSON::make_object()};
      branch.assign("disallow", std::move(negation));
      branches.push_back(std::move(branch));
    }

    schema.erase("disallow");

    if (schema.defines("extends") && schema.at("extends").is_array()) {
      this->extends_start_ = schema.at("extends").size();
      for (auto &branch : branches.as_array()) {
        schema.at("extends").push_back(std::move(branch));
      }
    } else if (schema.defines("extends")) {
      auto extends{sourcemeta::core::JSON::make_array()};
      extends.push_back(schema.at("extends"));
      this->extends_start_ = extends.size();
      for (auto &branch : branches.as_array()) {
        extends.push_back(std::move(branch));
      }
      schema.assign("extends", std::move(extends));
    } else {
      this->extends_start_ = 0;
      schema.assign("extends", std::move(branches));
    }
  }

  [[nodiscard]] auto rereference(const std::string_view,
                                 const sourcemeta::core::Pointer &,
                                 const sourcemeta::core::Pointer &target,
                                 const sourcemeta::core::Pointer &current) const
      -> std::optional<sourcemeta::core::Pointer> override {
    const auto disallow_prefix{current.concat("disallow")};
    if (!target.starts_with(disallow_prefix)) {
      return target;
    }

    const auto relative{target.resolve_from(disallow_prefix)};
    if (relative.empty() || !relative.at(0).is_index()) {
      return target;
    }

    const auto index{relative.at(0).to_index()};
    return target.rebase(
        current.concat(sourcemeta::core::Pointer{"disallow", index}),
        current.concat(
            {"extends", this->extends_start_ + index, "disallow", 0}));
  }

private:
  mutable std::size_t extends_start_{0};
};
