class Upgrade201909To202012 final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  Upgrade201909To202012()
      : SchemaTransformRule{"upgrade_2019_09_to_2020_12", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &root,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &frame,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &walker,
            const sourcemeta::blaze::SchemaResolver &resolver, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains(Vocabularies::Known::JSON_Schema_2019_09_Core) &&
        schema.is_object());

    const bool is_resource_scope{
        location.type ==
            sourcemeta::blaze::SchemaFrame::LocationType::Resource ||
        location.pointer.empty()};

    if (is_resource_scope) {
      compute_anchor_sanitization(root, frame, location);
      if (!this->anchor_renames_.empty() ||
          !this->anchor_ref_rewrites_.empty()) {
        this->descendant_has_pending_pattern_ =
            any_descendant_has_pending_pattern(root, frame, location);
        this->resource_has_recursive_anchor_ =
            compute_resource_has_recursive_anchor(root, frame, location);
        this->document_has_unevaluated_items_ =
            compute_document_has_unevaluated_items(root, frame, walker,
                                                   resolver);
        this->is_inside_contains_wrapper_ = false;
        return Result{std::vector<sourcemeta::core::Pointer>{},
                      std::string{"sanitize"}};
      }
    }

    if (enclosing_resource_has_pending_sanitization(root, frame, location)) {
      return false;
    }

    this->is_inside_contains_wrapper_ =
        location_inside_contains_wrapper(location);

    if (any_descendant_has_pending_pattern(root, frame, location)) {
      return false;
    }

    this->resource_has_recursive_anchor_ =
        compute_resource_has_recursive_anchor(root, frame, location);
    this->document_has_unevaluated_items_ =
        compute_document_has_unevaluated_items(root, frame, walker, resolver);
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema, const Result &result) const
      -> void override {
    const bool is_sanitize_phase{result.description.has_value() &&
                                 result.description.value() == "sanitize"};
    if (is_sanitize_phase) {
      apply_anchor_sanitization(schema);
      if (this->descendant_has_pending_pattern_) {
        return;
      }
    }

    this->renames_.clear();

    if (schema.defines("$recursiveAnchor") &&
        schema.at("$recursiveAnchor").is_boolean()) {
      if (schema.at("$recursiveAnchor").to_boolean()) {
        schema.rename("$recursiveAnchor", "$dynamicAnchor");
        schema.at("$dynamicAnchor").into(sourcemeta::core::JSON{"meta"});
      } else {
        schema.erase("$recursiveAnchor");
      }
    }

    if (schema.defines("$recursiveRef")) {
      schema.rename("$recursiveRef", "$dynamicRef");
      if (this->resource_has_recursive_anchor_) {
        schema.at("$dynamicRef").into(sourcemeta::core::JSON{"#meta"});
      }
    }

    if (schema.defines("items") && schema.at("items").is_array()) {
      if (schema.at("items").empty()) {
        schema.erase("items");
      } else {
        this->renames_.emplace_back(sourcemeta::core::Pointer{"items"},
                                    sourcemeta::core::Pointer{"prefixItems"});
        schema.rename("items", "prefixItems");
      }
      if (schema.defines("additionalItems")) {
        this->renames_.emplace_back(
            sourcemeta::core::Pointer{"additionalItems"},
            sourcemeta::core::Pointer{"items"});
        schema.rename("additionalItems", "items");
      }
    } else if (schema.defines("additionalItems")) {
      schema.erase("additionalItems");
    }

    if (schema.defines("contains") && !this->is_inside_contains_wrapper_ &&
        this->document_has_unevaluated_items_) {
      auto wrapper_inner{sourcemeta::core::JSON::make_object()};
      wrapper_inner.assign("contains", schema.at("contains"));
      if (schema.defines("minContains")) {
        wrapper_inner.assign("minContains", schema.at("minContains"));
        schema.erase("minContains");
      }
      if (schema.defines("maxContains")) {
        wrapper_inner.assign("maxContains", schema.at("maxContains"));
        schema.erase("maxContains");
      }

      auto inner_not{sourcemeta::core::JSON::make_object()};
      inner_not.assign("not", std::move(wrapper_inner));

      if (!schema.defines("not")) {
        schema.rename("contains", "not");
        schema.at("not").into(std::move(inner_not));
        this->renames_.emplace_back(
            sourcemeta::core::Pointer{"contains"},
            sourcemeta::core::Pointer{"not", "not", "contains"});
      } else {
        schema.erase("contains");
        auto outer_not{sourcemeta::core::JSON::make_object()};
        outer_not.assign("not", std::move(inner_not));
        if (schema.defines("allOf") && schema.at("allOf").is_array()) {
          const auto allof_index{schema.at("allOf").size()};
          schema.at("allOf").push_back(std::move(outer_not));
          this->renames_.emplace_back(
              sourcemeta::core::Pointer{"contains"},
              sourcemeta::core::Pointer{
                  "allOf",
                  static_cast<sourcemeta::core::Pointer::Token::Index>(
                      allof_index),
                  "not", "not", "contains"});
        } else {
          auto allof_array{sourcemeta::core::JSON::make_array()};
          allof_array.push_back(std::move(outer_not));
          schema.assign("allOf", std::move(allof_array));
          this->renames_.emplace_back(
              sourcemeta::core::Pointer{"contains"},
              sourcemeta::core::Pointer{
                  "allOf",
                  static_cast<sourcemeta::core::Pointer::Token::Index>(0),
                  "not", "not", "contains"});
        }
      }
    }

    rewrite_vocabulary(schema);

    if (schema.defines("$schema") && schema.at("$schema").is_string() &&
        schema.at("$schema").to_string() == DRAFT_2019_09_URL) {
      schema.assign("$schema", sourcemeta::core::JSON{DRAFT_2020_12_URL});
      drop_dialect_overrides(schema, true);
    } else {
      mark_dialect_override(schema, DRAFT_2020_12_URL);
    }
  }

  [[nodiscard]] auto rereference(const std::string_view,
                                 const sourcemeta::core::Pointer &,
                                 const sourcemeta::core::Pointer &target,
                                 const sourcemeta::core::Pointer &current) const
      -> sourcemeta::core::Pointer override {
    for (const auto &[old_pointer, new_pointer] : this->renames_) {
      const auto result{target.rebase(current.concat(old_pointer),
                                      current.concat(new_pointer))};
      if (result != target) {
        return result;
      }
    }

    return target;
  }

private:
  static constexpr std::string_view DRAFT_2019_09_URL{
      "https://json-schema.org/draft/2019-09/schema"};
  static constexpr std::string_view DRAFT_2020_12_URL{
      "https://json-schema.org/draft/2020-12/schema"};
  static constexpr std::string_view APPLICATOR_2019_09_URI{
      "https://json-schema.org/draft/2019-09/vocab/applicator"};
  static constexpr std::string_view APPLICATOR_2020_12_URI{
      "https://json-schema.org/draft/2020-12/vocab/applicator"};
  static constexpr std::string_view UNEVALUATED_2020_12_URI{
      "https://json-schema.org/draft/2020-12/vocab/unevaluated"};

  static inline const std::unordered_map<std::string, std::string>
      VOCAB_URI_MAP_2019_09_TO_2020_12{
          {"https://json-schema.org/draft/2019-09/vocab/core",
           "https://json-schema.org/draft/2020-12/vocab/core"},
          {"https://json-schema.org/draft/2019-09/vocab/applicator",
           "https://json-schema.org/draft/2020-12/vocab/applicator"},
          {"https://json-schema.org/draft/2019-09/vocab/validation",
           "https://json-schema.org/draft/2020-12/vocab/validation"},
          {"https://json-schema.org/draft/2019-09/vocab/meta-data",
           "https://json-schema.org/draft/2020-12/vocab/meta-data"},
          {"https://json-schema.org/draft/2019-09/vocab/format",
           "https://json-schema.org/draft/2020-12/vocab/format-annotation"},
          {"https://json-schema.org/draft/2019-09/vocab/content",
           "https://json-schema.org/draft/2020-12/vocab/content"}};

  static auto
  vocabulary_has_mappable_uri(const sourcemeta::core::JSON &subschema) -> bool {
    if (!subschema.is_object() || !subschema.defines("$vocabulary") ||
        !subschema.at("$vocabulary").is_object()) {
      return false;
    }
    for (const auto &entry : subschema.at("$vocabulary").as_object()) {
      if (VOCAB_URI_MAP_2019_09_TO_2020_12.contains(entry.first)) {
        return true;
      }
    }
    return false;
  }

  static auto rewrite_vocabulary(sourcemeta::core::JSON &schema) -> void {
    if (!schema.is_object() || !schema.defines("$vocabulary") ||
        !schema.at("$vocabulary").is_object()) {
      return;
    }

    std::unordered_set<std::string> source_keys;
    std::optional<sourcemeta::core::JSON> applicator_2019_09_value;
    for (const auto &entry : schema.at("$vocabulary").as_object()) {
      source_keys.insert(entry.first);
      if (entry.first == APPLICATOR_2019_09_URI) {
        applicator_2019_09_value = entry.second;
      }
    }

    const bool unevaluated_already_present{
        source_keys.contains(std::string{UNEVALUATED_2020_12_URI})};
    const bool should_inline_unevaluated{applicator_2019_09_value.has_value() &&
                                         !unevaluated_already_present};

    auto fresh{sourcemeta::core::JSON::make_object()};

    for (const auto &entry : schema.at("$vocabulary").as_object()) {
      const auto iter{VOCAB_URI_MAP_2019_09_TO_2020_12.find(entry.first)};
      if (iter == VOCAB_URI_MAP_2019_09_TO_2020_12.cend()) {
        fresh.assign(entry.first, entry.second);
        if (entry.first == APPLICATOR_2020_12_URI &&
            should_inline_unevaluated) {
          fresh.assign(std::string{UNEVALUATED_2020_12_URI},
                       applicator_2019_09_value.value());
        }
        continue;
      }

      if (source_keys.contains(iter->second)) {
        continue;
      }

      fresh.assign(iter->second, entry.second);

      if (entry.first == APPLICATOR_2019_09_URI && should_inline_unevaluated) {
        fresh.assign(std::string{UNEVALUATED_2020_12_URI}, entry.second);
      }
    }

    schema.at("$vocabulary").into(std::move(fresh));
  }

  struct AnchorRename {
    sourcemeta::core::Pointer subschema_pointer;
    std::string new_name;
  };

  struct AnchorRefRewrite {
    sourcemeta::core::Pointer ref_pointer;
    std::string new_value;
  };

  mutable std::vector<
      std::pair<sourcemeta::core::Pointer, sourcemeta::core::Pointer>>
      renames_;
  mutable bool resource_has_recursive_anchor_{false};
  mutable bool is_inside_contains_wrapper_{false};
  mutable bool descendant_has_pending_pattern_{false};
  mutable bool document_has_unevaluated_items_{false};
  mutable std::vector<AnchorRename> anchor_renames_;
  mutable std::vector<AnchorRefRewrite> anchor_ref_rewrites_;

  static auto compute_document_has_unevaluated_items(
      const sourcemeta::core::JSON &root,
      const sourcemeta::blaze::SchemaFrame &frame,
      const sourcemeta::blaze::SchemaWalker &walker,
      const sourcemeta::blaze::SchemaResolver &resolver) -> bool {
    for (const auto &entry : frame.locations()) {
      if (entry.second.type !=
              sourcemeta::blaze::SchemaFrame::LocationType::Resource &&
          entry.second.type !=
              sourcemeta::blaze::SchemaFrame::LocationType::Subschema) {
        continue;
      }
      const auto absolute{sourcemeta::core::to_pointer(entry.second.pointer)};
      const auto &subschema{sourcemeta::core::get(root, absolute)};
      if (!subschema.is_object() || !subschema.defines("unevaluatedItems")) {
        continue;
      }
      const auto location_vocabularies{
          frame.vocabularies(entry.second, resolver)};
      const auto &keyword_metadata{
          walker("unevaluatedItems", location_vocabularies)};
      if (keyword_metadata.type !=
          sourcemeta::blaze::SchemaKeywordType::Unknown) {
        return true;
      }
    }
    return false;
  }

  static auto any_descendant_has_pending_pattern(
      const sourcemeta::core::JSON &root,
      const sourcemeta::blaze::SchemaFrame &frame,
      const sourcemeta::blaze::SchemaFrame::Location &location) -> bool {
    for (const auto &entry : frame.locations()) {
      if (entry.second.type !=
              sourcemeta::blaze::SchemaFrame::LocationType::Resource &&
          entry.second.type !=
              sourcemeta::blaze::SchemaFrame::LocationType::Subschema) {
        continue;
      }
      if (entry.second.pointer.size() <= location.pointer.size() ||
          !entry.second.pointer.starts_with(location.pointer)) {
        continue;
      }
      const auto absolute{sourcemeta::core::to_pointer(entry.second.pointer)};
      const auto &descendant{sourcemeta::core::get(root, absolute)};
      if (has_pending_pattern(descendant, entry.second)) {
        return true;
      }
    }
    return false;
  }

  static auto is_2020_12_anchor_first_char(const char character) -> bool {
    return (character >= 'A' && character <= 'Z') ||
           (character >= 'a' && character <= 'z') || character == '_';
  }

  static auto is_2020_12_anchor_body_char(const char character) -> bool {
    return (character >= 'A' && character <= 'Z') ||
           (character >= 'a' && character <= 'z') ||
           (character >= '0' && character <= '9') || character == '_' ||
           character == '.' || character == '-';
  }

  static auto is_valid_2020_12_anchor(const std::string_view name) -> bool {
    if (name.empty()) {
      return false;
    }
    if (!is_2020_12_anchor_first_char(name.front())) {
      return false;
    }
    for (std::size_t index{1}; index < name.size(); ++index) {
      if (!is_2020_12_anchor_body_char(name[index])) {
        return false;
      }
    }
    return true;
  }

  static auto location_inside_contains_wrapper(
      const sourcemeta::blaze::SchemaFrame::Location &location) -> bool {
    if (location.pointer.size() < 2) {
      return false;
    }
    const auto &last{location.pointer.back()};
    if (!last.is_property() || last.to_property() != "not") {
      return false;
    }
    const auto &second_last{location.pointer.at(location.pointer.size() - 2)};
    if (!second_last.is_property() || second_last.to_property() != "not") {
      return false;
    }
    return true;
  }

  static auto
  has_pending_pattern(const sourcemeta::core::JSON &subschema,
                      const sourcemeta::blaze::SchemaFrame::Location &location)
      -> bool {
    if (!subschema.is_object()) {
      return false;
    }
    if (!subschema.defines_any({"$schema", "$recursiveAnchor", "$recursiveRef",
                                "items", "additionalItems", "contains",
                                "$vocabulary"})) {
      return false;
    }
    if (subschema.defines("$schema") && subschema.at("$schema").is_string() &&
        subschema.at("$schema").to_string() == DRAFT_2019_09_URL) {
      return true;
    }
    if (subschema.defines_any(
            {"$recursiveAnchor", "$recursiveRef", "additionalItems"})) {
      return true;
    }
    if (subschema.defines("items") && subschema.at("items").is_array()) {
      return true;
    }
    if (subschema.defines("contains") &&
        !location_inside_contains_wrapper(location)) {
      return true;
    }
    if (vocabulary_has_mappable_uri(subschema)) {
      return true;
    }
    return false;
  }

  static auto find_enclosing_resource(
      const sourcemeta::blaze::SchemaFrame &frame,
      const sourcemeta::blaze::SchemaFrame::Location &current_location)
      -> std::optional<std::reference_wrapper<
          const sourcemeta::blaze::SchemaFrame::Location>> {
    std::optional<
        std::reference_wrapper<const sourcemeta::blaze::SchemaFrame::Location>>
        closest;
    for (const auto &entry : frame.locations()) {
      const bool entry_is_resource_scope{
          entry.second.type ==
              sourcemeta::blaze::SchemaFrame::LocationType::Resource ||
          entry.second.pointer.empty()};
      if (!entry_is_resource_scope) {
        continue;
      }
      if (entry.second.pointer.size() > current_location.pointer.size()) {
        continue;
      }
      if (!current_location.pointer.starts_with(entry.second.pointer)) {
        continue;
      }
      if (!closest.has_value() ||
          entry.second.pointer.size() > closest.value().get().pointer.size()) {
        closest = std::cref(entry.second);
      }
    }
    return closest;
  }

  static auto
  pointer_within_resource(const sourcemeta::core::WeakPointer &candidate,
                          const sourcemeta::core::WeakPointer &resource_pointer)
      -> bool {
    if (!candidate.starts_with(resource_pointer)) {
      return false;
    }
    return true;
  }

  auto compute_anchor_sanitization(
      const sourcemeta::core::JSON &root,
      const sourcemeta::blaze::SchemaFrame &frame,
      const sourcemeta::blaze::SchemaFrame::Location &resource_location) const
      -> void {
    this->anchor_renames_.clear();
    this->anchor_ref_rewrites_.clear();

    const auto &resource_pointer{resource_location.pointer};

    std::set<std::string> existing_valid;
    std::vector<std::pair<std::string, sourcemeta::core::WeakPointer>> invalid;

    for (const auto &entry : frame.locations()) {
      if (entry.second.type !=
          sourcemeta::blaze::SchemaFrame::LocationType::Anchor) {
        continue;
      }
      if (entry.first.first != sourcemeta::blaze::SchemaReferenceType::Static) {
        continue;
      }
      if (!pointer_within_resource(entry.second.pointer, resource_pointer)) {
        continue;
      }
      const sourcemeta::core::URI anchor_uri{entry.first.second};
      const auto fragment{anchor_uri.fragment()};
      if (!fragment.has_value() || fragment.value().empty()) {
        continue;
      }
      const std::string anchor_name{fragment.value()};
      if (is_valid_2020_12_anchor(anchor_name)) {
        existing_valid.insert(anchor_name);
      } else {
        invalid.emplace_back(anchor_name, entry.second.pointer);
      }
    }

    if (invalid.empty()) {
      return;
    }

    static const AnchorCharPolicy POLICY{
        .is_valid_first = &is_2020_12_anchor_first_char,
        .is_valid_body = &is_2020_12_anchor_body_char};

    std::map<std::string, std::string> rename_map;
    std::set<std::string> in_use{existing_valid};
    for (const auto &[original, pointer] : invalid) {
      if (rename_map.contains(original)) {
        continue;
      }
      in_use.erase(original);
      const auto sanitized{
          sanitize_anchor_with_policy(original, in_use, POLICY)};
      rename_map.emplace(original, sanitized);
      in_use.insert(sanitized);
    }

    std::set<std::string> processed_pointers;
    for (const auto &[original, pointer] : invalid) {
      const auto pointer_str{sourcemeta::core::to_string(pointer)};
      if (processed_pointers.contains(pointer_str)) {
        continue;
      }
      processed_pointers.insert(pointer_str);

      const auto absolute{sourcemeta::core::to_pointer(pointer)};
      const auto &subschema{sourcemeta::core::get(root, absolute)};
      if (!subschema.is_object() || !subschema.defines("$anchor") ||
          !subschema.at("$anchor").is_string()) {
        continue;
      }
      const auto current_value{subschema.at("$anchor").to_string()};
      const auto rename_iter{rename_map.find(current_value)};
      if (rename_iter == rename_map.end()) {
        continue;
      }

      const auto relative_weak{pointer.resolve_from(resource_pointer)};
      this->anchor_renames_.push_back(
          {sourcemeta::core::to_pointer(relative_weak), rename_iter->second});
    }

    for (const auto &reference : frame.references()) {
      if (!pointer_within_resource(reference.first.second, resource_pointer)) {
        continue;
      }
      if (!reference.second.fragment.has_value()) {
        continue;
      }
      const auto &fragment{reference.second.fragment.value()};
      if (fragment.empty() || fragment.front() == '/') {
        continue;
      }
      const auto rename_iter{rename_map.find(std::string{fragment})};
      if (rename_iter == rename_map.end()) {
        continue;
      }

      sourcemeta::core::URI ref_uri{reference.second.original};
      ref_uri.fragment(rename_iter->second);
      const auto new_value{ref_uri.recompose()};

      const auto relative_weak{
          reference.first.second.resolve_from(resource_pointer)};
      this->anchor_ref_rewrites_.push_back(
          {sourcemeta::core::to_pointer(relative_weak), new_value});
    }
  }

  static auto enclosing_resource_has_pending_sanitization(
      const sourcemeta::core::JSON &root,
      const sourcemeta::blaze::SchemaFrame &frame,
      const sourcemeta::blaze::SchemaFrame::Location &current_location)
      -> bool {
    const auto closest{find_enclosing_resource(frame, current_location)};
    if (!closest.has_value()) {
      return false;
    }
    const auto &resource_pointer{closest.value().get().pointer};

    for (const auto &entry : frame.locations()) {
      if (entry.second.type !=
          sourcemeta::blaze::SchemaFrame::LocationType::Anchor) {
        continue;
      }
      if (entry.first.first != sourcemeta::blaze::SchemaReferenceType::Static) {
        continue;
      }
      if (!pointer_within_resource(entry.second.pointer, resource_pointer)) {
        continue;
      }
      const sourcemeta::core::URI anchor_uri{entry.first.second};
      const auto fragment{anchor_uri.fragment()};
      if (!fragment.has_value() || fragment.value().empty()) {
        continue;
      }
      if (!is_valid_2020_12_anchor(fragment.value())) {
        const auto absolute{sourcemeta::core::to_pointer(entry.second.pointer)};
        const auto &subschema{sourcemeta::core::get(root, absolute)};
        if (subschema.is_object() && subschema.defines("$anchor") &&
            subschema.at("$anchor").is_string() &&
            subschema.at("$anchor").to_string() == fragment.value()) {
          return true;
        }
      }
    }
    return false;
  }

  auto apply_anchor_sanitization(sourcemeta::core::JSON &schema) const -> void {
    for (const auto &rename : this->anchor_renames_) {
      auto &target{sourcemeta::core::get(schema, rename.subschema_pointer)};
      target.at("$anchor").into(sourcemeta::core::JSON{rename.new_name});
    }
    for (const auto &rewrite : this->anchor_ref_rewrites_) {
      auto &target{sourcemeta::core::get(schema, rewrite.ref_pointer)};
      target.into(sourcemeta::core::JSON{rewrite.new_value});
    }
  }

  static auto compute_resource_has_recursive_anchor(
      const sourcemeta::core::JSON &root,
      const sourcemeta::blaze::SchemaFrame &frame,
      const sourcemeta::blaze::SchemaFrame::Location &current_location)
      -> bool {
    const auto closest{find_enclosing_resource(frame, current_location)};
    if (!closest.has_value()) {
      return false;
    }

    const auto &resource_pointer{closest.value().get().pointer};
    std::set<std::string> seen;
    for (const auto &entry : frame.locations()) {
      if (entry.second.type !=
              sourcemeta::blaze::SchemaFrame::LocationType::Resource &&
          entry.second.type !=
              sourcemeta::blaze::SchemaFrame::LocationType::Subschema) {
        continue;
      }
      if (!entry.second.pointer.starts_with(resource_pointer)) {
        continue;
      }
      if (entry.second.type ==
              sourcemeta::blaze::SchemaFrame::LocationType::Resource &&
          entry.second.pointer.size() > resource_pointer.size()) {
        continue;
      }
      const auto pointer_str{sourcemeta::core::to_string(entry.second.pointer)};
      if (seen.contains(pointer_str)) {
        continue;
      }
      seen.insert(pointer_str);

      const auto absolute{sourcemeta::core::to_pointer(entry.second.pointer)};
      const auto &subschema{sourcemeta::core::get(root, absolute)};
      if (!subschema.is_object()) {
        continue;
      }
      if (subschema.defines("$recursiveAnchor") &&
          subschema.at("$recursiveAnchor").is_boolean() &&
          subschema.at("$recursiveAnchor").to_boolean()) {
        return true;
      }
    }
    return false;
  }
};
