class AllOfMergeCompatibleBranches final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  AllOfMergeCompatibleBranches()
      : SchemaTransformRule{"allof_merge_compatible_branches", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &walker,
            const sourcemeta::core::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    static const JSON::String KEYWORD{"allOf"};
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
                          Vocabularies::Known::JSON_Schema_2019_09_Applicator,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4}) &&
                     schema.is_object() && schema.defines(KEYWORD) &&
                     schema.at(KEYWORD).is_array() &&
                     schema.at(KEYWORD).size() >= 2);

    ONLY_CONTINUE_IF(!frame.has_references_through(
        location.pointer, WeakPointer::Token{std::cref(KEYWORD)}));

    const auto &branches{schema.at(KEYWORD)};

    for (std::size_t index_a = 0; index_a < branches.size(); ++index_a) {
      const auto &branch_a{branches.at(index_a)};
      if (!is_mergeable_branch(branch_a)) {
        continue;
      }

      for (std::size_t index_b = index_a + 1; index_b < branches.size();
           ++index_b) {
        const auto &branch_b{branches.at(index_b)};
        if (!is_mergeable_branch(branch_b)) {
          continue;
        }

        const auto a_is_type_only{is_type_only_branch(branch_a)};
        const auto b_is_type_only{is_type_only_branch(branch_b)};
        if (!a_is_type_only && !b_is_type_only) {
          continue;
        }

        const auto &non_type_branch{a_is_type_only ? branch_b : branch_a};
        if (has_in_place_applicators(non_type_branch)) {
          continue;
        }

        if (has_overlapping_keywords(branch_a, branch_b)) {
          continue;
        }

        if (has_cross_dependencies(branch_a, branch_b, walker, vocabularies)) {
          continue;
        }

        this->merge_into_ = index_a;
        this->merge_from_ = index_b;
        return true;
      }
    }

    return false;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    static const JSON::String KEYWORD{"allOf"};
    auto &target{schema.at(KEYWORD).at(this->merge_into_)};
    const auto &source{schema.at(KEYWORD).at(this->merge_from_)};
    target.merge(source.as_object());
    schema.at(KEYWORD).erase(
        std::next(schema.at(KEYWORD).as_array().begin(),
                  static_cast<std::ptrdiff_t>(this->merge_from_)));
  }

  [[nodiscard]] auto rereference(const std::string_view, const Pointer &,
                                 const Pointer &target,
                                 const Pointer &current) const
      -> Pointer override {
    static const JSON::String KEYWORD{"allOf"};
    const auto relative{target.resolve_from(current)};
    if (relative.size() < 2 || !relative.at(0).is_property() ||
        relative.at(0).to_property() != KEYWORD || !relative.at(1).is_index()) {
      return target;
    }

    const auto index{relative.at(1).to_index()};
    if (index == this->merge_from_) {
      const Pointer old_prefix{current.concat({KEYWORD, this->merge_from_})};
      const Pointer new_prefix{current.concat({KEYWORD, this->merge_into_})};
      return target.rebase(old_prefix, new_prefix);
    }

    if (index > this->merge_from_) {
      const Pointer old_prefix{current.concat({KEYWORD, index})};
      const Pointer new_prefix{current.concat({KEYWORD, index - 1})};
      return target.rebase(old_prefix, new_prefix);
    }

    return target;
  }

private:
  static auto is_type_only_branch(const JSON &branch) -> bool {
    return branch.size() == 1 && branch.defines("type");
  }

  static auto has_in_place_applicators(const JSON &branch) -> bool {
    return branch.defines("anyOf") || branch.defines("oneOf") ||
           branch.defines("allOf") || branch.defines("not") ||
           branch.defines("if");
  }

  static auto is_mergeable_branch(const JSON &branch) -> bool {
    if (!branch.is_object()) {
      return false;
    }
    return !branch.defines("$ref") && !branch.defines("$dynamicRef") &&
           !branch.defines("$recursiveRef") && !branch.defines("$id") &&
           !branch.defines("$schema") && !branch.defines("id") &&
           !branch.defines("$anchor") && !branch.defines("$dynamicAnchor") &&
           !branch.defines("$recursiveAnchor");
  }

  static auto has_overlapping_keywords(const JSON &branch_a,
                                       const JSON &branch_b) -> bool {
    for (const auto &entry : branch_a.as_object()) {
      if (branch_b.defines(entry.first)) {
        return true;
      }
    }
    return false;
  }

  static auto
  has_cross_dependencies(const JSON &branch_a, const JSON &branch_b,
                         const sourcemeta::core::SchemaWalker &walker,
                         const sourcemeta::core::Vocabularies &vocabularies)
      -> bool {
    for (const auto &entry_a : branch_a.as_object()) {
      const auto &metadata{walker(entry_a.first, vocabularies)};
      for (const auto &dependency : metadata.dependencies) {
        if (branch_b.defines(JSON::String{dependency})) {
          return true;
        }
      }
    }

    for (const auto &entry_b : branch_b.as_object()) {
      const auto &metadata{walker(entry_b.first, vocabularies)};
      for (const auto &dependency : metadata.dependencies) {
        if (branch_a.defines(JSON::String{dependency})) {
          return true;
        }
      }
    }

    return false;
  }

  mutable std::size_t merge_into_{0};
  mutable std::size_t merge_from_{0};
};
