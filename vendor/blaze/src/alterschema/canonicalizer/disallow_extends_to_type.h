class DisallowExtendsToType final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  DisallowExtendsToType()
      : SchemaTransformRule{
            "disallow_extends_to_type",
            "Negating a conjunction is the disjunction of the negations: an "
            "`extends` under `disallow` becomes a `type` union where each "
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
    ONLY_CONTINUE_IF(element.is_object() && element.defines("extends") &&
                     element.at("extends").is_array() &&
                     !element.at("extends").empty());

    // Only a pure negation can be distributed: the schema must assert nothing
    // besides `disallow` (otherwise the new `type` would clobber a sibling
    // constraint), and the negated schema must assert nothing besides `extends`
    // (otherwise those conjuncts would be silently dropped)
    ONLY_CONTINUE_IF(
        wraps_single_constraint(schema, "disallow", walker, vocabularies) &&
        wraps_single_constraint(element, "extends", walker, vocabularies));

    // The conjuncts relocate to distinct `type` branches (handled by
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
    for (auto &branch : schema.at("disallow").at(0).at("extends").as_array()) {
      auto negation{JSON::make_array()};
      negation.push_back(std::move(branch));
      auto element{JSON::make_object()};
      element.assign("disallow", std::move(negation));
      branches.push_back(std::move(element));
    }

    schema.erase("disallow");
    schema.assign("type", std::move(branches));
  }

  [[nodiscard]] auto rereference(const std::string_view, const Pointer &,
                                 const Pointer &target,
                                 const Pointer &current) const
      -> Pointer override {
    const auto extends_prefix{current.concat({"disallow", 0, "extends"})};
    if (!target.starts_with(extends_prefix)) {
      return target;
    }

    const auto relative{target.resolve_from(extends_prefix)};
    if (relative.empty() || !relative.at(0).is_index()) {
      return target;
    }

    const auto index{relative.at(0).to_index()};
    return target.rebase(extends_prefix.concat({index}),
                         current.concat({"type", index, "disallow", 0}));
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
