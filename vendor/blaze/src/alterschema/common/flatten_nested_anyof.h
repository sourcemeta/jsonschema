class FlattenNestedAnyOf final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  FlattenNestedAnyOf()
      : SchemaTransformRule{
            "flatten_nested_anyof",
            "An `anyOf` branch that only contains another `anyOf` can "
            "be flattened into the parent `anyOf`"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    static const JSON::String KEYWORD{"anyOf"};
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
                          Vocabularies::Known::JSON_Schema_2019_09_Applicator,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4}) &&
                     schema.is_object() && schema.defines(KEYWORD) &&
                     schema.at(KEYWORD).is_array());

    this->flatten_indices_.clear();
    const auto &branches{schema.at(KEYWORD)};
    for (std::size_t index = 0; index < branches.size(); ++index) {
      const auto &branch{branches.at(index)};
      if (branch.is_object() && branch.size() == 1 && branch.defines(KEYWORD) &&
          branch.at(KEYWORD).is_array()) {
        this->flatten_indices_.push_back(index);
      }
    }

    ONLY_CONTINUE_IF(!this->flatten_indices_.empty());
    ONLY_CONTINUE_IF(!frame.has_references_through(
        location.pointer, WeakPointer::Token{std::cref(KEYWORD)}));
    return APPLIES_TO_KEYWORDS(KEYWORD);
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    static const JSON::String KEYWORD{"anyOf"};
    this->index_mapping_.clear();
    const auto &original{schema.at(KEYWORD)};
    auto result{JSON::make_array()};
    std::size_t new_index{0};
    std::size_t flatten_cursor{0};

    for (std::size_t index = 0; index < original.size(); ++index) {
      if (flatten_cursor < this->flatten_indices_.size() &&
          this->flatten_indices_[flatten_cursor] == index) {
        this->collect_leaves_(original.at(index), KEYWORD, index, result,
                              new_index);
        ++flatten_cursor;
      } else {
        this->index_mapping_.emplace_back(index, std::nullopt, new_index);
        result.push_back(original.at(index));
        ++new_index;
      }
    }

    schema.assign(KEYWORD, std::move(result));
  }

  [[nodiscard]] auto rereference(const std::string_view, const Pointer &,
                                 const Pointer &target,
                                 const Pointer &current) const
      -> Pointer override {
    static const JSON::String KEYWORD{"anyOf"};
    const auto prefix{current.concat({KEYWORD})};
    if (!target.starts_with(prefix)) {
      return target;
    }
    const auto relative{target.resolve_from(prefix)};
    if (relative.empty() || !relative.at(0).is_index()) {
      return target;
    }
    const auto old_index{relative.at(0).to_index()};
    for (const auto &[outer, inner, mapped] : this->index_mapping_) {
      if (outer == old_index && inner.has_value()) {
        const Pointer old_prefix{
            prefix.concat({old_index, KEYWORD, inner.value()})};
        if (target.starts_with(old_prefix)) {
          const Pointer new_prefix{prefix.concat({mapped})};
          return target.rebase(old_prefix, new_prefix);
        }
      } else if (outer == old_index) {
        const Pointer old_prefix{prefix.concat({old_index})};
        const Pointer new_prefix{prefix.concat({mapped})};
        return target.rebase(old_prefix, new_prefix);
      }
    }
    return target;
  }

private:
  auto collect_leaves_(const JSON &node, const JSON::String &keyword,
                       std::size_t outer_index, JSON &result,
                       std::size_t &new_index) const -> void {
    const auto &inner{node.at(keyword)};
    for (std::size_t inner_index = 0; inner_index < inner.size();
         ++inner_index) {
      const auto &child{inner.at(inner_index)};
      if (child.is_object() && child.size() == 1 && child.defines(keyword) &&
          child.at(keyword).is_array()) {
        this->collect_leaves_(child, keyword, outer_index, result, new_index);
      } else {
        this->index_mapping_.emplace_back(outer_index, inner_index, new_index);
        result.push_back(child);
        ++new_index;
      }
    }
  }

  mutable std::vector<std::size_t> flatten_indices_;
  mutable std::vector<
      std::tuple<std::size_t, std::optional<std::size_t>, std::size_t>>
      index_mapping_;
};
