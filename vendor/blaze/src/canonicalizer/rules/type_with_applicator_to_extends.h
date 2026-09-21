class TypeWithApplicatorToExtends final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  TypeWithApplicatorToExtends()
      : SchemaTransformRule{"type_with_applicator_to_extends"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_0,
                          SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1,
                          SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_2,
                          SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3}) &&
                     schema.is_object());

    const auto *extends_value{schema.try_at("extends")};
    const bool has_extends{(extends_value != nullptr) &&
                           extends_value->is_array()};
    const auto *disallow_value{schema.try_at("disallow")};
    const bool has_disallow{(disallow_value != nullptr) &&
                            disallow_value->is_array()};
    const auto *type_value{schema.try_at("type")};
    const bool has_type_array{(type_value != nullptr) &&
                              type_value->is_array()};
    const bool has_type{(type_value != nullptr) && type_value->is_string()};
    const bool has_enum{schema.defines("enum")};
    const unsigned int applicator_count{(has_extends ? 1U : 0U) +
                                        (has_disallow ? 1U : 0U) +
                                        (has_type_array ? 1U : 0U)};

    // An assertion left beside `extends` or `disallow` has to be pushed into a
    // branch of its own, otherwise shapes such as `extends` sitting next to
    // `properties` are never rewritten and the result is not in canonical form.
    //
    // A type union on its own deliberately does not count here. In Draft 3 the
    // distribution rule folds neighbouring assertions into the branches, but in
    // Draft 0 to 2 there is no such rule, so `{"type": [...], "minLength": 2}`
    // is a resting state. Splitting it would only invite the implicit type rule
    // to rebuild the union around the assertion on the next pass, and the
    // fixpoint would never settle
    const bool has_assertion{
        std::ranges::any_of(schema.as_object(), [](const auto &entry) -> bool {
          return is_assertion_keyword(entry.first) &&
                 !(entry.first == "type" && entry.second.is_array());
        })};
    const bool has_structural{has_type || has_enum ||
                              ((has_extends || has_disallow) && has_assertion)};

    ONLY_CONTINUE_IF((has_structural && applicator_count >= 1) ||
                     applicator_count >= 2);
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    this->typed_keywords_.clear();

    auto typed_branch{sourcemeta::core::JSON::make_object()};
    for (const auto &entry : schema.as_object()) {
      // Note that `required` must stay where it is. It is a marker read by the
      // parent `properties` object rather than an assertion about the instance,
      // so moving it into an `extends` branch silently makes it inert and the
      // implicit default left behind flips whether the property is required
      if (entry.first == "extends" || entry.first == "disallow" ||
          entry.first == "$schema" || entry.first == "id" ||
          entry.first == "required" ||
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
    if (schema.defines("required")) {
      new_schema.assign("required", schema.at("required"));
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
    static const sourcemeta::core::JSON::String EXTENDS_KEYWORD{"extends"};

    for (const auto &typed_keyword : this->typed_keywords_) {
      if (typed_keyword == keyword) {
        const sourcemeta::core::Pointer old_prefix{current.concat(keyword)};
        const std::size_t typed_index{
            static_cast<std::size_t>(std::popcount(this->applicator_indices_))};
        const sourcemeta::core::Pointer new_prefix{
            current.concat({EXTENDS_KEYWORD, typed_index, keyword})};
        return target.rebase(old_prefix, new_prefix);
      }
    }

    std::size_t index{0};
    for (const auto &applicator : APPLICATORS) {
      if (keyword == applicator) {
        const sourcemeta::core::Pointer old_prefix{current.concat(keyword)};
        const sourcemeta::core::Pointer new_prefix{
            current.concat({EXTENDS_KEYWORD, index, keyword})};
        return target.rebase(old_prefix, new_prefix);
      }
      if ((this->applicator_indices_ & applicator_bit(applicator)) != 0) {
        index++;
      }
    }

    return target;
  }

private:
  static constexpr std::array<const char *, 3> APPLICATORS{
      {"extends", "disallow", "type"}};

  // The assertion keywords of these dialects. This is deliberately an
  // allow-list rather than a list of keywords to ignore: a keyword missing from
  // here only means the rule does not fire, which is the previous behaviour,
  // whereas a keyword wrongly treated as an assertion is rewritten on every
  // pass and the fixpoint never settles. Markers such as `required` and
  // `optional`, the `definitions` container, the core identity keywords and
  // metadata are all absent for that reason
  static constexpr std::array<const char *, 19> ASSERTIONS{
      {"enum", "type", "properties", "patternProperties",
       "additionalProperties", "items", "additionalItems", "minItems",
       "maxItems", "uniqueItems", "minLength", "maxLength", "pattern", "format",
       "divisibleBy", "minimum", "maximum", "exclusiveMinimum",
       "exclusiveMaximum"}};

  static auto is_assertion_keyword(std::string_view keyword) -> bool {
    return std::ranges::any_of(
        ASSERTIONS,
        [keyword](const auto *entry) -> bool { return keyword == entry; });
  }

  static constexpr auto applicator_bit(std::string_view keyword)
      -> std::uint8_t {
    if (keyword == "extends") {
      return 1;
    }
    if (keyword == "disallow") {
      return 2;
    }
    if (keyword == "type") {
      return 4;
    }
    return 0;
  }

  mutable std::vector<std::string> typed_keywords_;
  mutable std::uint8_t applicator_indices_{0};
};
