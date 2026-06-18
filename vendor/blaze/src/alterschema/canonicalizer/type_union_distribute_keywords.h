class TypeUnionDistributeKeywords final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  TypeUnionDistributeKeywords()
      : SchemaTransformRule{
            "type_union_distribute_keywords",
            "A type-specific keyword sibling to a `type` union belongs inside "
            "the branch of the type that it applies to"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &walker,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_Draft_3,
                          Vocabularies::Known::JSON_Schema_Draft_3_Hyper}) &&
                     schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type && type->is_array() && !type->empty());
    for (const auto &branch : type->as_array()) {
      ONLY_CONTINUE_IF(branch.is_object());
    }

    this->moves_.clear();
    this->wrap_keywords_.clear();
    this->wrap_ = false;
    std::vector<JSON::String> movable;
    for (const auto &entry : schema.as_object()) {
      // `required` is a property-presence flag, not a value assertion, so it
      // is never pushed into a branch
      if (entry.first == "type" || entry.first == "required") {
        continue;
      }

      const auto &metadata{walker(entry.first, vocabularies)};
      if (metadata.type == sourcemeta::blaze::SchemaKeywordType::Reference) {
        continue;
      }

      // A keyword that applies to every type carries no type-specific
      // information to push down into a branch
      if (metadata.instances.none()) {
        continue;
      }

      movable.push_back(entry.first);

      std::vector<std::size_t> targets;
      bool has_match{false};
      bool conflict{false};
      for (std::size_t index = 0; index < type->size(); ++index) {
        const auto branch_types{branch_type_set(type->at(index))};
        if ((branch_types & metadata.instances).none()) {
          continue;
        }

        has_match = true;
        // A matching branch already constrains this keyword, so distributing
        // and erasing the sibling could drop the top-level bound from that
        // branch. Wrap instead so nothing is weakened.
        if (type->at(index).defines(entry.first)) {
          conflict = true;
          break;
        }

        targets.push_back(index);
      }

      if (!has_match || conflict) {
        this->wrap_ = true;
      } else {
        this->moves_.emplace_back(entry.first, std::move(targets));
      }
    }

    ONLY_CONTINUE_IF(!movable.empty());
    if (this->wrap_) {
      this->moves_.clear();
      this->wrap_keywords_ = std::move(movable);
    }

    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    if (this->wrap_) {
      auto union_branch{JSON::make_object()};
      union_branch.assign("type", schema.at("type"));
      auto sibling_branch{JSON::make_object()};
      for (const auto &keyword : this->wrap_keywords_) {
        sibling_branch.assign(keyword, schema.at(keyword));
      }

      schema.erase("type");
      for (const auto &keyword : this->wrap_keywords_) {
        schema.erase(keyword);
      }

      if (schema.defines("extends") && schema.at("extends").is_array()) {
        this->type_index_ = schema.at("extends").size();
        schema.at("extends").push_back(std::move(union_branch));
        this->sibling_index_ = schema.at("extends").size();
        schema.at("extends").push_back(std::move(sibling_branch));
      } else {
        auto extends{JSON::make_array()};
        this->type_index_ = 0;
        extends.push_back(std::move(union_branch));
        this->sibling_index_ = 1;
        extends.push_back(std::move(sibling_branch));
        schema.assign("extends", std::move(extends));
      }

      return;
    }

    for (const auto &entry : this->moves_) {
      const auto value{schema.at(entry.first)};
      auto &type{schema.at("type")};
      for (const auto index : entry.second) {
        type.at(index).assign(entry.first, value);
      }
    }

    for (const auto &entry : this->moves_) {
      schema.erase(entry.first);
    }
  }

  [[nodiscard]] auto rereference(const std::string_view, const Pointer &,
                                 const Pointer &target,
                                 const Pointer &current) const
      -> Pointer override {
    if (this->wrap_) {
      const auto type_prefix{current.concat({"type"})};
      if (target.starts_with(type_prefix)) {
        return target.rebase(
            type_prefix,
            current.concat({"extends", this->type_index_, "type"}));
      }

      for (const auto &keyword : this->wrap_keywords_) {
        const auto keyword_prefix{current.concat({keyword})};
        if (target.starts_with(keyword_prefix)) {
          return target.rebase(
              keyword_prefix,
              current.concat({"extends", this->sibling_index_, keyword}));
        }
      }

      return target;
    }

    for (const auto &entry : this->moves_) {
      if (entry.second.empty()) {
        continue;
      }

      const auto keyword_prefix{current.concat({entry.first})};
      if (target.starts_with(keyword_prefix)) {
        return target.rebase(
            keyword_prefix,
            current.concat({"type", entry.second.front(), entry.first}));
      }
    }

    return target;
  }

private:
  static auto branch_type_set(const sourcemeta::core::JSON &branch)
      -> sourcemeta::core::JSON::TypeSet {
    if (!branch.is_object()) {
      return {};
    }

    const auto *type{branch.try_at("type")};
    if (type && (type->is_string() || type->is_array())) {
      return parse_schema_type(*type);
    }

    const auto *enum_value{branch.try_at("enum")};
    if (enum_value && enum_value->is_array()) {
      sourcemeta::core::JSON::TypeSet result;
      for (const auto &value : enum_value->as_array()) {
        result.set(std::to_underlying(value.type()));
      }
      return result;
    }

    return {};
  }

  mutable std::vector<
      std::pair<sourcemeta::core::JSON::String, std::vector<std::size_t>>>
      moves_;
  mutable std::vector<sourcemeta::core::JSON::String> wrap_keywords_;
  mutable bool wrap_{false};
  mutable std::size_t type_index_{0};
  mutable std::size_t sibling_index_{0};
};
