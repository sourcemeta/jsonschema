class EnumDropRedundantValidation final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  EnumDropRedundantValidation()
      : SchemaTransformRule{"enum_drop_redundant_validation", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &walker,
            const sourcemeta::core::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_Draft_0,
             Vocabularies::Known::JSON_Schema_Draft_1,
             Vocabularies::Known::JSON_Schema_Draft_2,
             Vocabularies::Known::JSON_Schema_Draft_3,
             Vocabularies::Known::JSON_Schema_Draft_4,
             Vocabularies::Known::JSON_Schema_Draft_6,
             Vocabularies::Known::JSON_Schema_Draft_7,
             Vocabularies::Known::JSON_Schema_2019_09_Validation,
             Vocabularies::Known::JSON_Schema_2020_12_Validation}) &&
        schema.is_object() && schema.defines("enum") &&
        schema.at("enum").is_array() && !schema.defines("type"));

    this->keywords_.clear();
    this->wrap_keywords_.clear();
    this->is_pre_draft4_ =
        vocabularies.contains_any({Vocabularies::Known::JSON_Schema_Draft_0,
                                   Vocabularies::Known::JSON_Schema_Draft_1,
                                   Vocabularies::Known::JSON_Schema_Draft_2,
                                   Vocabularies::Known::JSON_Schema_Draft_3});
    this->has_if_group_ =
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_Draft_7,
             Vocabularies::Known::JSON_Schema_2019_09_Applicator,
             Vocabularies::Known::JSON_Schema_2020_12_Applicator}) &&
        schema.defines("if");
    for (const auto &entry : schema.as_object()) {
      if (entry.first == "enum") {
        continue;
      }

      if (this->has_if_group_ &&
          (entry.first == "then" || entry.first == "else")) {
        continue;
      }

      const auto &metadata{walker(entry.first, vocabularies)};
      if (metadata.type == sourcemeta::core::SchemaKeywordType::Unknown ||
          metadata.type == sourcemeta::core::SchemaKeywordType::Annotation ||
          metadata.type == sourcemeta::core::SchemaKeywordType::Other ||
          metadata.type == sourcemeta::core::SchemaKeywordType::Comment ||
          metadata.type ==
              sourcemeta::core::SchemaKeywordType::LocationMembers) {
        continue;
      }

      if (entry.second.is_boolean() && entry.second.to_boolean()) {
        if (!frame.has_references_through(
                location.pointer, WeakPointer::Token{std::cref(entry.first)})) {
          this->keywords_.emplace_back(entry.first);
        }
        continue;
      }

      if (entry.second.is_object() && entry.second.empty()) {
        this->keywords_.emplace_back(entry.first);
        continue;
      }

      if (!frame.has_references_through(
              location.pointer, WeakPointer::Token{std::cref(entry.first)})) {
        this->wrap_keywords_.emplace_back(entry.first);
      }
    }

    ONLY_CONTINUE_IF(!this->keywords_.empty() || !this->wrap_keywords_.empty());
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    for (const auto &keyword : this->keywords_) {
      schema.erase(keyword);
    }

    if (this->wrap_keywords_.empty()) {
      return;
    }

    auto new_allof{JSON::make_array()};
    for (const auto &keyword : this->wrap_keywords_) {
      auto branch{JSON::make_object()};
      branch.assign(keyword, schema.at(keyword));
      if (keyword == "if" && this->has_if_group_) {
        if (schema.defines("then")) {
          branch.assign("then", schema.at("then"));
        }
        if (schema.defines("else")) {
          branch.assign("else", schema.at("else"));
        }
      }
      new_allof.push_back(std::move(branch));
      schema.erase(keyword);
      if (keyword == "if" && this->has_if_group_) {
        if (schema.defines("then")) {
          schema.erase("then");
        }
        if (schema.defines("else")) {
          schema.erase("else");
        }
      }
    }

    auto enum_branch{JSON::make_object()};
    enum_branch.assign("enum", schema.at("enum"));
    schema.erase("enum");
    new_allof.push_back(std::move(enum_branch));

    const auto wrapper_keyword{this->is_pre_draft4_ ? "extends" : "allOf"};
    schema.assign(wrapper_keyword, std::move(new_allof));
  }

private:
  mutable std::vector<JSON::String> keywords_;
  mutable std::vector<JSON::String> wrap_keywords_;
  mutable bool has_if_group_{false};
  mutable bool is_pre_draft4_{false};
};
