class SingleBranchOneOf final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  SingleBranchOneOf() : SchemaTransformRule{"single_branch_oneof", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    static const JSON::String KEYWORD{"oneOf"};
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
                          Vocabularies::Known::JSON_Schema_2019_09_Applicator,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4}) &&
                     schema.is_object() && schema.defines(KEYWORD) &&
                     schema.at(KEYWORD).is_array() &&
                     schema.at(KEYWORD).size() == 1);
    ONLY_CONTINUE_IF(!frame.has_references_through(
        location.pointer, WeakPointer::Token{std::cref(KEYWORD)}));
    this->has_unevaluated_ =
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Unevaluated,
             Vocabularies::Known::JSON_Schema_2019_09_Applicator}) &&
        (schema.defines("unevaluatedProperties") ||
         schema.defines("unevaluatedItems"));
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    if (this->has_unevaluated_) {
      schema.rename("oneOf", "allOf");
      return;
    }

    auto &branch{schema.at("oneOf").at(0)};
    if (branch.is_boolean()) {
      if (branch.to_boolean()) {
        schema.erase("oneOf");
      } else {
        schema.into(JSON{false});
      }
      return;
    }

    schema.merge(branch.as_object());
    schema.erase("oneOf");
  }

  [[nodiscard]] auto rereference(const std::string_view, const Pointer &,
                                 const Pointer &target,
                                 const Pointer &current) const
      -> Pointer override {
    static const JSON::String KEYWORD{"oneOf"};
    if (this->has_unevaluated_) {
      const auto old_prefix{current.concat({KEYWORD})};
      const Pointer new_prefix{current.concat({"allOf"})};
      return target.rebase(old_prefix, new_prefix);
    }
    const auto prefix{current.concat({KEYWORD, 0})};
    if (!target.starts_with(prefix)) {
      return target;
    }
    const auto relative{target.resolve_from(prefix)};
    if (relative.empty()) {
      return target;
    }
    const Pointer new_prefix{current.concat({relative.at(0)})};
    return target.rebase(prefix.concat({relative.at(0)}), new_prefix);
  }

private:
  mutable bool has_unevaluated_{false};
};
