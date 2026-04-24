class TypeWithApplicatorToAllOf final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  TypeWithApplicatorToAllOf()
      : SchemaTransformRule{"type_with_applicator_to_allof", ""} {};

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
            {Vocabularies::Known::JSON_Schema_Draft_4,
             Vocabularies::Known::JSON_Schema_Draft_6,
             Vocabularies::Known::JSON_Schema_Draft_7,
             Vocabularies::Known::JSON_Schema_2019_09_Applicator,
             Vocabularies::Known::JSON_Schema_2020_12_Applicator}) &&
        schema.is_object());

    const bool has_not{schema.defines("not")};
    const bool has_anyof{schema.defines("anyOf")};
    const bool has_allof{schema.defines("allOf")};
    const bool has_oneof{schema.defines("oneOf")};
    const bool has_if{
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_Draft_7,
             Vocabularies::Known::JSON_Schema_2019_09_Applicator,
             Vocabularies::Known::JSON_Schema_2020_12_Applicator}) &&
        schema.defines("if")};
    this->has_if_then_else_ = has_if;
    const bool has_type{schema.defines("type") &&
                        schema.at("type").is_string()};
    const bool has_enum{schema.defines("enum")};
    const bool is_modern{
        vocabularies.contains(Vocabularies::Known::JSON_Schema_2019_09_Core) ||
        vocabularies.contains(Vocabularies::Known::JSON_Schema_2020_12_Core)};
    const bool has_ref{!is_modern && schema.defines("$ref")};
    this->has_modern_ref_ = is_modern && schema.defines("$ref");
    this->has_dynamic_ref_ =
        vocabularies.contains(Vocabularies::Known::JSON_Schema_2020_12_Core) &&
        schema.defines("$dynamicRef");
    this->has_recursive_ref_ =
        vocabularies.contains(Vocabularies::Known::JSON_Schema_2019_09_Core) &&
        schema.defines("$recursiveRef");
    const unsigned int applicator_count{
        (has_not ? 1U : 0U) + (has_anyof ? 1U : 0U) + (has_allof ? 1U : 0U) +
        (has_oneof ? 1U : 0U) + (has_if ? 1U : 0U) +
        (this->has_modern_ref_ ? 1U : 0U) + (this->has_dynamic_ref_ ? 1U : 0U) +
        (this->has_recursive_ref_ ? 1U : 0U)};
    const bool has_structural{has_type || has_enum || has_ref};

    bool modern_ref_needs_wrapping{false};
    this->ref_annotations_only_ = false;
    if (this->has_modern_ref_ || this->has_dynamic_ref_ ||
        this->has_recursive_ref_) {
      this->ref_annotations_only_ = true;
      for (const auto &entry : schema.as_object()) {
        if (entry.first == "$ref" || entry.first == "$dynamicRef" ||
            entry.first == "$recursiveRef") {
          continue;
        }
        const auto keyword_type{walker(entry.first, vocabularies).type};
        if (keyword_type != sourcemeta::core::SchemaKeywordType::Unknown &&
            keyword_type != sourcemeta::core::SchemaKeywordType::Annotation &&
            keyword_type != sourcemeta::core::SchemaKeywordType::Comment) {
          modern_ref_needs_wrapping = true;
          if (keyword_type != sourcemeta::core::SchemaKeywordType::Reference &&
              keyword_type != sourcemeta::core::SchemaKeywordType::Other &&
              keyword_type !=
                  sourcemeta::core::SchemaKeywordType::LocationMembers) {
            this->ref_annotations_only_ = false;
          }
        }
      }
    }

    this->has_unevaluated_ =
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Unevaluated,
             Vocabularies::Known::JSON_Schema_2019_09_Applicator}) &&
        (schema.defines("unevaluatedProperties") ||
         schema.defines("unevaluatedItems"));
    bool has_orphaned_typed_keywords{false};
    if (is_modern && applicator_count >= 1 && !has_structural) {
      for (const auto &entry : schema.as_object()) {
        if (entry.first == "unevaluatedProperties" ||
            entry.first == "unevaluatedItems") {
          continue;
        }
        const auto &metadata{walker(entry.first, vocabularies)};
        if (metadata.instances.any()) {
          has_orphaned_typed_keywords = true;
          break;
        }
      }
    }

    ONLY_CONTINUE_IF(
        (has_structural && applicator_count >= 1) || applicator_count >= 2 ||
        modern_ref_needs_wrapping ||
        (has_orphaned_typed_keywords && !this->ref_annotations_only_));

    this->strategy_ = Strategy::FullRestructure;
    this->applicators_with_refs_ = 0;
    for (const auto &reference : frame.references()) {
      const auto &source_pointer{reference.first.second};
      if (!source_pointer.starts_with(location.pointer)) {
        continue;
      }
      const auto relative{source_pointer.resolve_from(location.pointer)};
      if (relative.empty() || !relative.at(0).is_property()) {
        continue;
      }
      const auto &first_keyword{relative.at(0).to_property()};
      const auto bit{applicator_bit(first_keyword)};
      if (bit != 0) {
        const auto destination{frame.traverse(reference.second.destination)};
        if (destination.has_value()) {
          const auto &dest_pointer{destination->get().pointer};
          if (dest_pointer.starts_with(location.pointer)) {
            const auto relative_dest{
                dest_pointer.resolve_from(location.pointer)};
            if (!relative_dest.empty() && relative_dest.at(0).is_property() &&
                (relative_dest.at(0).to_property() == "definitions" ||
                 relative_dest.at(0).to_property() == "$defs" ||
                 relative_dest.at(0).to_property() == "dependencies" ||
                 relative_dest.at(0).to_property() == "dependentSchemas")) {
              continue;
            }
          } else {
            continue;
          }
        }
        this->strategy_ = Strategy::SafeExtract;
        this->applicators_with_refs_ |= bit;
      }
    }

    if (this->strategy_ == Strategy::SafeExtract && !has_structural) {
      if (!has_allof) {
        this->strategy_ = Strategy::FullRestructure;
        return true;
      }

      bool all_refs_fixed{true};
      for (const auto &reference : frame.references()) {
        const auto &source_pointer{reference.first.second};
        if (!source_pointer.starts_with(location.pointer)) {
          continue;
        }
        const auto relative_src{source_pointer.resolve_from(location.pointer)};
        if (relative_src.empty() || !relative_src.at(0).is_property()) {
          continue;
        }
        const auto &src_keyword{relative_src.at(0).to_property()};
        if (src_keyword != "not" && src_keyword != "anyOf" &&
            src_keyword != "oneOf" &&
            !(this->has_if_then_else_ &&
              (src_keyword == "if" || src_keyword == "then" ||
               src_keyword == "else"))) {
          continue;
        }

        const auto destination{frame.traverse(reference.second.destination)};
        if (!destination.has_value()) {
          all_refs_fixed = false;
          break;
        }

        const auto relative_dest{
            destination->get().pointer.resolve_from(location.pointer)};
        if (relative_dest.empty() || !relative_dest.at(0).is_property() ||
            relative_dest.at(0).to_property() != "allOf") {
          all_refs_fixed = false;
          break;
        }
      }

      if (all_refs_fixed) {
        this->strategy_ = Strategy::MergeIntoAllOf;
      } else {
        return false;
      }
    }

    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    this->typed_keywords_.clear();

    if (this->strategy_ == Strategy::MergeIntoAllOf) {
      for (const auto &applicator : APPLICATORS_WITHOUT_ALLOF) {
        if (!schema.defines(applicator)) {
          continue;
        }
        auto branch{JSON::make_object()};
        branch.assign(applicator, schema.at(applicator));
        if (std::string_view{applicator} == "if" && this->has_if_then_else_) {
          if (schema.defines("then")) {
            branch.assign("then", schema.at("then"));
          }
          if (schema.defines("else")) {
            branch.assign("else", schema.at("else"));
          }
        }
        schema.at("allOf").push_back(std::move(branch));
        schema.erase(applicator);
      }
      if (this->has_if_then_else_) {
        if (schema.defines("then")) {
          schema.erase("then");
        }
        if (schema.defines("else")) {
          schema.erase("else");
        }
      }
      return;
    }

    auto typed_branch{JSON::make_object()};
    for (const auto &entry : schema.as_object()) {
      if (entry.first == "not" || entry.first == "anyOf" ||
          entry.first == "allOf" || entry.first == "oneOf" ||
          entry.first == "$schema" || entry.first == "id" ||
          entry.first == "$id" || entry.first == "definitions" ||
          entry.first == "$defs" || entry.first == "$anchor" ||
          entry.first == "$dynamicAnchor" ||
          entry.first == "$recursiveAnchor" || entry.first == "$vocabulary" ||
          entry.first == "dependencies" || entry.first == "dependentSchemas" ||
          (this->has_if_then_else_ &&
           (entry.first == "if" || entry.first == "then" ||
            entry.first == "else")) ||
          (this->has_modern_ref_ && entry.first == "$ref") ||
          (this->has_dynamic_ref_ && entry.first == "$dynamicRef") ||
          (this->has_recursive_ref_ && entry.first == "$recursiveRef") ||
          (this->has_unevaluated_ && (entry.first == "unevaluatedProperties" ||
                                      entry.first == "unevaluatedItems"))) {
        continue;
      }
      typed_branch.assign(entry.first, entry.second);
      this->typed_keywords_.emplace_back(entry.first);
    }

    for (const auto &key : this->typed_keywords_) {
      schema.erase(key);
    }

    if (this->strategy_ == Strategy::SafeExtract) {
      if (schema.defines("allOf") && schema.at("allOf").is_array()) {
        this->typed_branch_index_ = schema.at("allOf").size();
        schema.at("allOf").push_back(std::move(typed_branch));
      } else {
        this->typed_branch_index_ = 0;
        auto new_allof{JSON::make_array()};
        new_allof.push_back(std::move(typed_branch));
        schema.assign("allOf", std::move(new_allof));
      }

      if (this->has_modern_ref_ && schema.defines("$ref")) {
        auto branch{JSON::make_object()};
        branch.assign("$ref", schema.at("$ref"));
        schema.at("allOf").push_back(std::move(branch));
        schema.erase("$ref");
      }
      if (this->has_dynamic_ref_ && schema.defines("$dynamicRef")) {
        auto branch{JSON::make_object()};
        branch.assign("$dynamicRef", schema.at("$dynamicRef"));
        schema.at("allOf").push_back(std::move(branch));
        schema.erase("$dynamicRef");
      }
      if (this->has_recursive_ref_ && schema.defines("$recursiveRef")) {
        auto branch{JSON::make_object()};
        branch.assign("$recursiveRef", schema.at("$recursiveRef"));
        schema.at("allOf").push_back(std::move(branch));
        schema.erase("$recursiveRef");
      }

      for (const auto &applicator : APPLICATORS_WITHOUT_ALLOF) {
        if (!schema.defines(applicator)) {
          continue;
        }
        if (this->applicators_with_refs_ & applicator_bit(applicator)) {
          continue;
        }
        auto branch{JSON::make_object()};
        branch.assign(applicator, schema.at(applicator));
        if (std::string_view{applicator} == "if" && this->has_if_then_else_) {
          if (schema.defines("then")) {
            branch.assign("then", schema.at("then"));
          }
          if (schema.defines("else")) {
            branch.assign("else", schema.at("else"));
          }
        }
        schema.at("allOf").push_back(std::move(branch));
        schema.erase(applicator);
        if (std::string_view{applicator} == "if" && this->has_if_then_else_) {
          if (schema.defines("then")) {
            schema.erase("then");
          }
          if (schema.defines("else")) {
            schema.erase("else");
          }
        }
      }

      return;
    }

    auto new_allof{JSON::make_array()};
    this->applicator_indices_ = 0;

    if (this->has_modern_ref_ && schema.defines("$ref")) {
      auto branch{JSON::make_object()};
      branch.assign("$ref", schema.at("$ref"));
      if (this->ref_annotations_only_ && !this->typed_keywords_.empty()) {
        for (const auto &entry : typed_branch.as_object()) {
          branch.assign(entry.first, entry.second);
        }
        this->typed_keywords_.clear();
      }
      new_allof.push_back(std::move(branch));
    }
    if (this->has_dynamic_ref_ && schema.defines("$dynamicRef")) {
      auto branch{JSON::make_object()};
      branch.assign("$dynamicRef", schema.at("$dynamicRef"));
      if (this->ref_annotations_only_ && !this->typed_keywords_.empty()) {
        for (const auto &entry : typed_branch.as_object()) {
          branch.assign(entry.first, entry.second);
        }
        this->typed_keywords_.clear();
      }
      new_allof.push_back(std::move(branch));
    }
    if (this->has_recursive_ref_ && schema.defines("$recursiveRef")) {
      auto branch{JSON::make_object()};
      branch.assign("$recursiveRef", schema.at("$recursiveRef"));
      if (this->ref_annotations_only_ && !this->typed_keywords_.empty()) {
        for (const auto &entry : typed_branch.as_object()) {
          branch.assign(entry.first, entry.second);
        }
        this->typed_keywords_.clear();
      }
      new_allof.push_back(std::move(branch));
    }

    for (const auto &applicator : APPLICATORS) {
      if (!schema.defines(applicator)) {
        continue;
      }
      auto branch{JSON::make_object()};
      branch.assign(applicator, schema.at(applicator));
      if (std::string_view{applicator} == "if" && this->has_if_then_else_) {
        if (schema.defines("then")) {
          branch.assign("then", schema.at("then"));
        }
        if (schema.defines("else")) {
          branch.assign("else", schema.at("else"));
        }
      }
      new_allof.push_back(std::move(branch));
      this->applicator_indices_ |= applicator_bit(applicator);
    }

    if (!this->typed_keywords_.empty()) {
      new_allof.push_back(std::move(typed_branch));
    }

    auto new_schema{JSON::make_object()};
    if (schema.defines("$schema")) {
      new_schema.assign("$schema", schema.at("$schema"));
    }
    if (schema.defines("id")) {
      new_schema.assign("id", schema.at("id"));
    }
    if (schema.defines("$id")) {
      new_schema.assign("$id", schema.at("$id"));
    }
    if (schema.defines("definitions")) {
      new_schema.assign("definitions", schema.at("definitions"));
    }
    if (schema.defines("$defs")) {
      new_schema.assign("$defs", schema.at("$defs"));
    }
    if (schema.defines("$anchor")) {
      new_schema.assign("$anchor", schema.at("$anchor"));
    }
    if (schema.defines("$dynamicAnchor")) {
      new_schema.assign("$dynamicAnchor", schema.at("$dynamicAnchor"));
    }
    if (schema.defines("$recursiveAnchor")) {
      new_schema.assign("$recursiveAnchor", schema.at("$recursiveAnchor"));
    }
    if (schema.defines("$vocabulary")) {
      new_schema.assign("$vocabulary", schema.at("$vocabulary"));
    }
    if (schema.defines("dependencies")) {
      new_schema.assign("dependencies", schema.at("dependencies"));
    }
    if (schema.defines("dependentSchemas")) {
      new_schema.assign("dependentSchemas", schema.at("dependentSchemas"));
    }
    if (this->has_unevaluated_) {
      if (schema.defines("unevaluatedProperties")) {
        new_schema.assign("unevaluatedProperties",
                          schema.at("unevaluatedProperties"));
      }
      if (schema.defines("unevaluatedItems")) {
        new_schema.assign("unevaluatedItems", schema.at("unevaluatedItems"));
      }
    }
    new_schema.assign("allOf", std::move(new_allof));
    schema.into(std::move(new_schema));
  }

  [[nodiscard]] auto rereference(const std::string_view, const Pointer &,
                                 const Pointer &target,
                                 const Pointer &current) const
      -> Pointer override {
    const auto relative{target.resolve_from(current)};
    if (relative.empty() || !relative.at(0).is_property()) {
      return target;
    }

    const auto &keyword{relative.at(0).to_property()};
    static const JSON::String allof_keyword{"allOf"};

    for (const auto &typed_kw : this->typed_keywords_) {
      if (typed_kw == keyword) {
        const Pointer old_prefix{current.concat({keyword})};
        if (this->strategy_ == Strategy::SafeExtract) {
          const Pointer new_prefix{current.concat(
              {allof_keyword, this->typed_branch_index_, keyword})};
          return target.rebase(old_prefix, new_prefix);
        } else {
          const std::size_t typed_index{(this->has_modern_ref_ ? 1U : 0U) +
                                        (this->has_dynamic_ref_ ? 1U : 0U) +
                                        (this->has_recursive_ref_ ? 1U : 0U) +
                                        static_cast<std::size_t>(std::popcount(
                                            this->applicator_indices_))};
          const Pointer new_prefix{
              current.concat({allof_keyword, typed_index, keyword})};
          return target.rebase(old_prefix, new_prefix);
        }
      }
    }

    if (this->strategy_ == Strategy::FullRestructure) {
      std::size_t index{0};
      if (this->has_modern_ref_) {
        index++;
      }
      if (this->has_dynamic_ref_) {
        index++;
      }
      if (this->has_recursive_ref_) {
        index++;
      }

      for (const auto &applicator : APPLICATORS) {
        if (keyword == applicator ||
            (this->has_if_then_else_ && std::string_view{applicator} == "if" &&
             (keyword == "then" || keyword == "else"))) {
          const Pointer old_prefix{current.concat({keyword})};
          const Pointer new_prefix{
              current.concat({allof_keyword, index, keyword})};
          return target.rebase(old_prefix, new_prefix);
        }
        if (this->applicator_indices_ & applicator_bit(applicator)) {
          index++;
        }
      }
    }

    return target;
  }

private:
  static constexpr std::array<const char *, 5> APPLICATORS{
      {"not", "anyOf", "allOf", "oneOf", "if"}};
  static constexpr std::array<const char *, 4> APPLICATORS_WITHOUT_ALLOF{
      {"not", "anyOf", "oneOf", "if"}};

  static constexpr auto applicator_bit(std::string_view keyword)
      -> std::uint8_t {
    if (keyword == "not")
      return 1;
    if (keyword == "anyOf")
      return 2;
    if (keyword == "allOf")
      return 4;
    if (keyword == "oneOf")
      return 8;
    if (keyword == "if" || keyword == "then" || keyword == "else")
      return 16;
    return 0;
  }

  enum class Strategy : std::uint8_t {
    FullRestructure,
    SafeExtract,
    MergeIntoAllOf
  };
  mutable Strategy strategy_{Strategy::FullRestructure};
  mutable bool has_if_then_else_{false};
  mutable bool has_modern_ref_{false};
  mutable bool has_dynamic_ref_{false};
  mutable bool has_recursive_ref_{false};
  mutable bool has_unevaluated_{false};
  mutable bool ref_annotations_only_{false};
  mutable std::vector<std::string> typed_keywords_;
  mutable std::uint8_t applicator_indices_{0};
  mutable std::uint8_t applicators_with_refs_{0};
  mutable std::size_t typed_branch_index_{0};
};
