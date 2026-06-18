class DisallowTypeUnionToExtends final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  DisallowTypeUnionToExtends()
      : SchemaTransformRule{
            "disallow_type_union_to_extends",
            "Negating a disjunction is the conjunction of the negations: a "
            "`type` union under `disallow` becomes an `extends` where each "
            "branch is its own single negation"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &frame,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &walker,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_Draft_3,
                          Vocabularies::Known::JSON_Schema_Draft_3_Hyper}) &&
                     schema.is_object());

    const auto *disallow{schema.try_at("disallow")};
    ONLY_CONTINUE_IF(disallow && disallow->is_array() && disallow->size() == 1);

    const auto &element{disallow->at(0)};
    ONLY_CONTINUE_IF(element.is_object() && element.defines("type") &&
                     element.at("type").is_array() &&
                     !element.at("type").empty());

    // Only a pure negation can be distributed: the schema must assert nothing
    // besides `disallow` (otherwise the new `extends` would clobber a sibling
    // constraint), and the negated schema must assert nothing besides its
    // `type` union (otherwise those conjuncts would be silently dropped)
    ONLY_CONTINUE_IF(
        wraps_single_constraint(schema, "disallow", walker, vocabularies) &&
        wraps_single_constraint(element, "type", walker, vocabularies));

    // The union members relocate to distinct `extends` branches (handled by
    // `rereference`), but the wrapper schema itself is dissolved rather than
    // moved, so a reference straight at it has no new home: bail in that case
    static const JSON::String DISALLOW{"disallow"};
    auto wrapper_pointer{location.pointer};
    wrapper_pointer.push_back(std::cref(DISALLOW));
    wrapper_pointer.push_back(static_cast<std::size_t>(0));
    ONLY_CONTINUE_IF(!frame.has_references_to(wrapper_pointer));

    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    auto branches{JSON::make_array()};
    for (auto &member : schema.at("disallow").at(0).at("type").as_array()) {
      auto negation{JSON::make_array()};
      negation.push_back(std::move(member));
      auto branch{JSON::make_object()};
      branch.assign("disallow", std::move(negation));
      branches.push_back(std::move(branch));
    }

    schema.erase("disallow");
    schema.assign("extends", std::move(branches));
  }

  [[nodiscard]] auto rereference(const std::string_view, const Pointer &,
                                 const Pointer &target,
                                 const Pointer &current) const
      -> Pointer override {
    const auto type_prefix{current.concat({"disallow", 0, "type"})};
    if (!target.starts_with(type_prefix)) {
      return target;
    }

    const auto relative{target.resolve_from(type_prefix)};
    if (relative.empty() || !relative.at(0).is_index()) {
      return target;
    }

    const auto index{relative.at(0).to_index()};
    return target.rebase(type_prefix.concat({index}),
                         current.concat({"extends", index, "disallow", 0}));
  }

private:
  static auto wraps_single_constraint(
      const sourcemeta::core::JSON &schema, const std::string_view keyword,
      const sourcemeta::blaze::SchemaWalker &walker,
      const sourcemeta::blaze::Vocabularies &vocabularies) -> bool {
    for (const auto &entry : schema.as_object()) {
      if (entry.first == keyword) {
        continue;
      }

      const auto type{walker(entry.first, vocabularies).type};
      if (type != SchemaKeywordType::Annotation &&
          type != SchemaKeywordType::Comment &&
          type != SchemaKeywordType::Other &&
          type != SchemaKeywordType::Unknown &&
          type != SchemaKeywordType::LocationMembers) {
        return false;
      }
    }

    return true;
  }
};
