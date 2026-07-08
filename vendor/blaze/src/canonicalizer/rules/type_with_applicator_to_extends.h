class TypeWithApplicatorToExtends final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  TypeWithApplicatorToExtends()
      : SchemaTransformRule{"type_with_applicator_to_extends"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any({Vocabularies::Known::JSON_Schema_Draft_0,
                                   Vocabularies::Known::JSON_Schema_Draft_1,
                                   Vocabularies::Known::JSON_Schema_Draft_2,
                                   Vocabularies::Known::JSON_Schema_Draft_3}) &&
        schema.is_object());

    const auto *extends_value{schema.try_at("extends")};
    const bool has_extends{extends_value && extends_value->is_array()};
    const auto *disallow_value{schema.try_at("disallow")};
    const bool has_disallow{disallow_value && disallow_value->is_array()};
    const auto *type_value{schema.try_at("type")};
    const bool has_type_array{type_value && type_value->is_array()};
    const bool has_type{type_value && type_value->is_string()};
    const bool has_enum{schema.defines("enum")};
    const unsigned int applicator_count{(has_extends ? 1U : 0U) +
                                        (has_disallow ? 1U : 0U) +
                                        (has_type_array ? 1U : 0U)};
    const bool has_structural{has_type || has_enum};

    ONLY_CONTINUE_IF((has_structural && applicator_count >= 1) ||
                     applicator_count >= 2);
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    this->typed_keywords_.clear();

    auto typed_branch{sourcemeta::core::JSON::make_object()};
    for (const auto &entry : schema.as_object()) {
      if (entry.first == "extends" || entry.first == "disallow" ||
          entry.first == "$schema" || entry.first == "id" ||
          (entry.first == "type" && entry.second.is_array())) {
        continue;
      }
      typed_branch.assign(entry.first, entry.second);
      this->typed_keywords_.emplace_back(entry.first);
    }

    for (const auto &key : this->typed_keywords_) {
      schema.erase(key);
    }

    auto new_extends{sourcemeta::core::JSON::make_array()};
    this->applicator_indices_ = 0;

    for (const auto &applicator : APPLICATORS) {
      if (!schema.defines(applicator)) {
        continue;
      }
      const auto &value{schema.at(applicator)};
      if (std::string_view{applicator} == "type" && !value.is_array()) {
        continue;
      }
      auto branch{sourcemeta::core::JSON::make_object()};
      branch.assign(applicator, value);
      new_extends.push_back(std::move(branch));
      this->applicator_indices_ |= applicator_bit(applicator);
    }

    if (!this->typed_keywords_.empty()) {
      new_extends.push_back(std::move(typed_branch));
    }

    auto new_schema{sourcemeta::core::JSON::make_object()};
    if (schema.defines("$schema")) {
      new_schema.assign("$schema", schema.at("$schema"));
    }
    if (schema.defines("id")) {
      new_schema.assign("id", schema.at("id"));
    }
    new_schema.assign("extends", std::move(new_extends));
    schema.into(std::move(new_schema));
  }

  [[nodiscard]] auto rereference(const std::string_view,
                                 const sourcemeta::core::Pointer &,
                                 const sourcemeta::core::Pointer &target,
                                 const sourcemeta::core::Pointer &current) const
      -> std::optional<sourcemeta::core::Pointer> override {
    const auto relative{target.resolve_from(current)};
    if (relative.empty() || !relative.at(0).is_property()) {
      return target;
    }

    const auto &keyword{relative.at(0).to_property()};
    static const sourcemeta::core::JSON::String extends_keyword{"extends"};

    for (const auto &typed_keyword : this->typed_keywords_) {
      if (typed_keyword == keyword) {
        const sourcemeta::core::Pointer old_prefix{current.concat(keyword)};
        const std::size_t typed_index{
            static_cast<std::size_t>(std::popcount(this->applicator_indices_))};
        const sourcemeta::core::Pointer new_prefix{
            current.concat({extends_keyword, typed_index, keyword})};
        return target.rebase(old_prefix, new_prefix);
      }
    }

    std::size_t index{0};
    for (const auto &applicator : APPLICATORS) {
      if (keyword == applicator) {
        const sourcemeta::core::Pointer old_prefix{current.concat(keyword)};
        const sourcemeta::core::Pointer new_prefix{
            current.concat({extends_keyword, index, keyword})};
        return target.rebase(old_prefix, new_prefix);
      }
      if (this->applicator_indices_ & applicator_bit(applicator)) {
        index++;
      }
    }

    return target;
  }

private:
  static constexpr std::array<const char *, 3> APPLICATORS{
      {"extends", "disallow", "type"}};

  static constexpr auto applicator_bit(std::string_view keyword)
      -> std::uint8_t {
    if (keyword == "extends")
      return 1;
    if (keyword == "disallow")
      return 2;
    if (keyword == "type")
      return 4;
    return 0;
  }

  mutable std::vector<std::string> typed_keywords_;
  mutable std::uint8_t applicator_indices_{0};
};
