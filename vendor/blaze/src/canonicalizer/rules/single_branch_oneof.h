class SingleBranchOneOf final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  SingleBranchOneOf() : SchemaTransformRule{"single_branch_oneof"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &frame,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    static const sourcemeta::core::JSON::String KEYWORD{"oneOf"};
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
                          Vocabularies::Known::JSON_Schema_2019_09_Applicator,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4}) &&
                     schema.is_object());

    const auto *one_of{schema.try_at(KEYWORD)};
    ONLY_CONTINUE_IF(one_of && one_of->is_array() && one_of->size() == 1);
    ONLY_CONTINUE_IF(!frame.has_references_through(
        location.pointer,
        sourcemeta::core::WeakPointer::Token{std::cref(KEYWORD)}));
    this->has_unevaluated_ =
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Unevaluated,
             Vocabularies::Known::JSON_Schema_2019_09_Applicator}) &&
        (schema.defines("unevaluatedProperties") ||
         schema.defines("unevaluatedItems"));
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    if (this->has_unevaluated_) {
      schema.rename("oneOf", "allOf");
      return;
    }

    auto &branch{schema.at("oneOf").at(0)};
    if (branch.is_boolean()) {
      if (branch.to_boolean()) {
        schema.erase("oneOf");
      } else {
        schema.into(sourcemeta::core::JSON{false});
      }
      return;
    }

    schema.merge(branch.as_object());
    schema.erase("oneOf");
  }

  [[nodiscard]] auto rereference(const std::string_view,
                                 const sourcemeta::core::Pointer &,
                                 const sourcemeta::core::Pointer &target,
                                 const sourcemeta::core::Pointer &current) const
      -> std::optional<sourcemeta::core::Pointer> override {
    static const sourcemeta::core::JSON::String KEYWORD{"oneOf"};
    if (this->has_unevaluated_) {
      const auto old_prefix{current.concat(KEYWORD)};
      const sourcemeta::core::Pointer new_prefix{current.concat("allOf")};
      return target.rebase(old_prefix, new_prefix);
    }
    const auto prefix{current.concat(sourcemeta::core::Pointer{KEYWORD, 0})};
    if (!target.starts_with(prefix)) {
      return target;
    }
    const auto relative{target.resolve_from(prefix)};
    if (relative.empty()) {
      return target;
    }
    const sourcemeta::core::Pointer new_prefix{
        current.concat({relative.at(0)})};
    return target.rebase(prefix.concat({relative.at(0)}), new_prefix);
  }

private:
  mutable bool has_unevaluated_{false};
};
