class SingleBranchAllOf final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  SingleBranchAllOf() : SchemaTransformRule{"single_branch_allof"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &frame,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    static const sourcemeta::core::JSON::String KEYWORD{"allOf"};
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
                          Vocabularies::Known::JSON_Schema_2019_09_Applicator,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4}) &&
                     schema.is_object());

    const auto *all_of{schema.try_at(KEYWORD)};
    ONLY_CONTINUE_IF(all_of && all_of->is_array() && all_of->size() == 1);
    ONLY_CONTINUE_IF(
        !(vocabularies.contains_any(
              {Vocabularies::Known::JSON_Schema_2020_12_Unevaluated,
               Vocabularies::Known::JSON_Schema_2019_09_Applicator}) &&
          (schema.defines("unevaluatedProperties") ||
           schema.defines("unevaluatedItems"))));
    ONLY_CONTINUE_IF(!frame.has_references_through(
        location.pointer,
        sourcemeta::core::WeakPointer::Token{std::cref(KEYWORD)}));
    const auto &branch{all_of->at(0)};
    if (branch.is_object()) {
      ONLY_CONTINUE_IF(!branch.defines("$ref") &&
                       !branch.defines("$dynamicRef") &&
                       !branch.defines("$recursiveRef"));
    }
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    auto &branch{schema.at("allOf").at(0)};
    if (branch.is_boolean()) {
      if (branch.to_boolean()) {
        schema.erase("allOf");
      } else {
        schema.into(sourcemeta::core::JSON{false});
      }
      return;
    }

    schema.merge(branch.as_object());
    schema.erase("allOf");
  }

  [[nodiscard]] auto rereference(const std::string_view,
                                 const sourcemeta::core::Pointer &,
                                 const sourcemeta::core::Pointer &target,
                                 const sourcemeta::core::Pointer &current) const
      -> std::optional<sourcemeta::core::Pointer> override {
    static const sourcemeta::core::JSON::String KEYWORD{"allOf"};
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
};
