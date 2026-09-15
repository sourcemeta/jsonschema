class TypeUnionDistributeKeywords final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  TypeUnionDistributeKeywords()
      : SchemaTransformRule{"type_union_distribute_keywords"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &walker,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3_HYPER}) &&
        schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type && type->is_array() && !type->empty());
    for (const auto &branch : type->as_array()) {
      ONLY_CONTINUE_IF(branch.is_object());
    }

    this->moves_.clear();
    this->wrap_keywords_.clear();
    this->wrap_ = false;
    std::vector<sourcemeta::core::JSON::String> movable;
    // Walking the frame is linear on the entire document, so only do it if a
    // keyword really is about to be copied into more than one branch, and
    // remember the outcome for the keywords that follow
    std::optional<std::vector<std::string_view>> declaring;
    for (const auto &entry : schema.as_object()) {
      // `required` is a property-presence flag, not a value assertion, so it
      // is never pushed into a branch
      if (entry.first == "type" || entry.first == "required") {
        continue;
      }

      const auto &metadata{walker(entry.first, vocabularies)};
      if (metadata.type == sourcemeta::core::SchemaKeywordType::Reference) {
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

        // `additionalProperties` and `additionalItems` only apply to what
        // their siblings do not already cover, so moving one of them next to
        // different siblings, or a sibling next to one of them, would change
        // what it covers
        if (changes_leftovers(entry.first, entry.second, type->at(index))) {
          conflict = true;
          break;
        }

        targets.push_back(index);
      }

      // Copying a value that declares an identifier or an anchor into more
      // than one branch would declare it more than once. Wrap instead so each
      // declaration is only ever moved
      bool duplicates_identifier{false};
      if (!this->wrap_ && targets.size() > 1) {
        if (!declaring.has_value()) {
          declaring = declaring_keywords(frame, location.pointer);
        }

        duplicates_identifier =
            std::ranges::contains(declaring.value(), entry.first);
      }

      if (!has_match || conflict || duplicates_identifier) {
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

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    if (this->wrap_) {
      auto union_branch{sourcemeta::core::JSON::make_object()};
      union_branch.assign("type", schema.at("type"));
      auto sibling_branch{sourcemeta::core::JSON::make_object()};
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
        auto extends{sourcemeta::core::JSON::make_array()};
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

  [[nodiscard]] auto rereference(const std::string_view,
                                 const sourcemeta::core::Pointer &,
                                 const sourcemeta::core::Pointer &target,
                                 const sourcemeta::core::Pointer &current) const
      -> std::optional<sourcemeta::core::Pointer> override {
    if (this->wrap_) {
      const auto type_prefix{current.concat("type")};
      if (target.starts_with(type_prefix)) {
        return target.rebase(
            type_prefix,
            current.concat({"extends", this->type_index_, "type"}));
      }

      for (const auto &keyword : this->wrap_keywords_) {
        const auto keyword_prefix{current.concat(keyword)};
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

      const auto keyword_prefix{current.concat(entry.first)};
      if (target.starts_with(keyword_prefix)) {
        return target.rebase(
            keyword_prefix,
            current.concat({"type", entry.second.front(), entry.first}));
      }
    }

    return target;
  }

private:
  // Whether the given keyword takes properties away from
  // `additionalProperties`. An empty one takes nothing away
  static auto narrows(const sourcemeta::core::JSON &schema,
                      const sourcemeta::core::JSON::String &keyword) -> bool {
    const auto *value{schema.try_at(keyword)};
    return (value != nullptr) && value->is_object() && !value->empty();
  }

  // Whether the given keyword of the given schema still constrains anything
  static auto constrains(const sourcemeta::core::JSON &schema,
                         const sourcemeta::core::JSON::String &keyword)
      -> bool {
    const auto *value{schema.try_at(keyword)};
    return (value != nullptr) && !is_empty_schema(*value);
  }

  // `additionalProperties` and `additionalItems` constrain whatever their
  // siblings leave over, so what they cover depends on the company they keep.
  // Vacuous siblings leave everything over, and a vacuous leftovers keyword
  // accepts whatever reaches it, so neither of those changes anything
  static auto changes_leftovers(const sourcemeta::core::JSON::String &keyword,
                                const sourcemeta::core::JSON &value,
                                const sourcemeta::core::JSON &branch) -> bool {
    if (keyword == "additionalProperties") {
      return !is_empty_schema(value) && (narrows(branch, "properties") ||
                                         narrows(branch, "patternProperties"));
    }

    if (keyword == "properties" || keyword == "patternProperties") {
      return value.is_object() && !value.empty() &&
             constrains(branch, "additionalProperties");
    }

    // A dormant `additionalItems` normally gets dropped when its schema has
    // no tuple `items`, but not while a reference points through it. Moving
    // such a one next to a tuple `items` would wake it up
    if (keyword == "additionalItems") {
      const auto *items{branch.try_at("items")};
      return !is_empty_schema(value) && (items != nullptr) && items->is_array();
    }

    if (keyword == "items") {
      return value.is_array() && constrains(branch, "additionalItems");
    }

    return false;
  }

  // The keywords of the given schema whose value declares an identifier or an
  // anchor anywhere inside it
  static auto declaring_keywords(const sourcemeta::core::SchemaFrame &frame,
                                 const sourcemeta::core::WeakPointer &base)
      -> std::vector<std::string_view> {
    std::vector<std::string_view> result;
    frame.for_each_location(
        [&base, &result](
            const sourcemeta::core::SchemaReferenceType, const std::string_view,
            const sourcemeta::core::SchemaFrame::Location &candidate) -> void {
          if ((candidate.type ==
                   sourcemeta::core::SchemaFrame::LocationType::Resource ||
               candidate.type ==
                   sourcemeta::core::SchemaFrame::LocationType::Anchor) &&
              candidate.pointer.size() > base.size() &&
              candidate.pointer.starts_with(base) &&
              candidate.pointer.at(base.size()).is_property()) {
            const std::string_view keyword{
                candidate.pointer.at(base.size()).to_property()};
            if (!std::ranges::contains(result, keyword)) {
              result.push_back(keyword);
            }
          }
        });
    return result;
  }

  static auto branch_type_set(const sourcemeta::core::JSON &branch)
      -> sourcemeta::core::JSON::TypeSet {
    if (!branch.is_object()) {
      return {};
    }

    const auto *type{branch.try_at("type")};
    if ((type != nullptr) && (type->is_string() || type->is_array())) {
      return parse_schema_type(*type);
    }

    const auto *enum_value{branch.try_at("enum")};
    if ((enum_value != nullptr) && enum_value->is_array()) {
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
