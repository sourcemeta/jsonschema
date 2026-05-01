class UpgradeDraft4ToDraft6 final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  UpgradeDraft4ToDraft6()
      : SchemaTransformRule{"upgrade_draft_4_to_draft_6", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &root,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains(Vocabularies::Known::JSON_Schema_Draft_4) &&
        schema.is_object());

    const bool is_resource_scope =
        location.type ==
            sourcemeta::core::SchemaFrame::LocationType::Resource ||
        location.pointer.empty();

    const bool sanitization_branch =
        is_resource_scope && resource_needs_anchor_sanitization(schema);

    const bool other_branch = has_pending_draft_4_pattern(schema);

    ONLY_CONTINUE_IF(sanitization_branch || other_branch);

    if (!sanitization_branch && other_branch &&
        enclosing_resource_has_pending_sanitization(location, root, frame)) {
      return false;
    }

    if (!sanitization_branch) {
      for (const auto &entry : frame.locations()) {
        if (entry.second.type !=
                sourcemeta::core::SchemaFrame::LocationType::Resource &&
            entry.second.type !=
                sourcemeta::core::SchemaFrame::LocationType::Subschema) {
          continue;
        }

        if (!is_strict_descendant(location.pointer, entry.second.pointer)) {
          continue;
        }

        const auto entry_pointer{
            sourcemeta::core::to_pointer(entry.second.pointer)};
        const auto &entry_schema{sourcemeta::core::get(root, entry_pointer)};

        if (entry_schema.is_object() && entry_schema.defines("$ref")) {
          continue;
        }

        if (has_pending_draft_4_pattern(entry_schema)) {
          return false;
        }
      }
    }

    if (sanitization_branch) {
      return Result{std::vector<sourcemeta::core::Pointer>{},
                    std::string{"sanitize"}};
    }

    return true;
  }

  auto transform(sourcemeta::core::JSON &schema, const Result &result) const
      -> void override {
    if (result.description.has_value()) {
      const auto renames{build_resource_rename_map(schema)};
      std::optional<std::string> resource_base;
      if (schema.defines("id") && schema.at("id").is_string()) {
        const sourcemeta::core::URI uri{schema.at("id").to_string()};
        const auto without_fragment{uri.recompose_without_fragment()};
        if (without_fragment.has_value() && !without_fragment.value().empty()) {
          resource_base = without_fragment.value();
        }
      }
      apply_anchor_renames_in_resource(schema, true, renames, resource_base);
      if (resource_has_descendant_with_pending_pattern(schema, true)) {
        return;
      }
    }

    if (schema.defines("id") && schema.at("id").is_string() &&
        !schema.defines("$id")) {
      schema.rename("id", "$id");
    }

    if (schema.defines("exclusiveMinimum") &&
        schema.at("exclusiveMinimum").is_boolean()) {
      const bool exclusive{schema.at("exclusiveMinimum").to_boolean()};
      schema.erase("exclusiveMinimum");
      if (exclusive && schema.defines("minimum") &&
          schema.at("minimum").is_number()) {
        schema.rename("minimum", "exclusiveMinimum");
      }
    }

    if (schema.defines("exclusiveMaximum") &&
        schema.at("exclusiveMaximum").is_boolean()) {
      const bool exclusive{schema.at("exclusiveMaximum").to_boolean()};
      schema.erase("exclusiveMaximum");
      if (exclusive && schema.defines("maximum") &&
          schema.at("maximum").is_number()) {
        schema.rename("maximum", "exclusiveMaximum");
      }
    }

    if (schema.defines("$schema") && schema.at("$schema").is_string() &&
        schema.at("$schema").to_string() == DRAFT_4_URL) {
      schema.assign("$schema", sourcemeta::core::JSON{DRAFT_6_URL});
      drop_dialect_overrides(schema, true);
    } else {
      mark_dialect_override(schema, DRAFT_6_URL);
    }
  }

private:
  static inline const std::string DRAFT_4_URL{
      "http://json-schema.org/draft-04/schema#"};
  static inline const std::string DRAFT_6_URL{
      "http://json-schema.org/draft-06/schema#"};
  static inline const std::array<std::string_view, 4> PROMOTED_KEYWORDS{
      {"const", "contains", "propertyNames", "examples"}};

  static auto
  has_pending_draft_4_pattern(const sourcemeta::core::JSON &subschema) -> bool {
    if (!subschema.is_object()) {
      return false;
    }

    if (subschema.defines("$schema") && subschema.at("$schema").is_string() &&
        subschema.at("$schema").to_string() == DRAFT_4_URL) {
      return true;
    }

    if (subschema.defines("id") && subschema.at("id").is_string() &&
        !subschema.defines("$id")) {
      const auto fragment{extract_id_fragment(subschema.at("id"))};
      if (!fragment.has_value() || fragment.value().empty() ||
          is_strict_plain_name(fragment.value())) {
        return true;
      }
    }

    const auto *exclusive_minimum{subschema.try_at("exclusiveMinimum")};
    if (exclusive_minimum != nullptr && exclusive_minimum->is_boolean()) {
      return true;
    }

    const auto *exclusive_maximum{subschema.try_at("exclusiveMaximum")};
    if (exclusive_maximum != nullptr && exclusive_maximum->is_boolean()) {
      return true;
    }

    for (const auto &keyword : PROMOTED_KEYWORDS) {
      if (subschema.defines(std::string{keyword})) {
        return true;
      }
    }

    return false;
  }

  static auto
  is_strict_descendant(const sourcemeta::core::WeakPointer &ancestor,
                       const sourcemeta::core::WeakPointer &candidate) -> bool {
    if (candidate.size() <= ancestor.size()) {
      return false;
    }
    for (std::size_t index{0}; index < ancestor.size(); ++index) {
      if (!(ancestor.at(index) == candidate.at(index))) {
        return false;
      }
    }
    return true;
  }

  static auto is_strict_plain_name_first_char(const char character) -> bool {
    return (character >= 'A' && character <= 'Z') ||
           (character >= 'a' && character <= 'z');
  }

  static auto is_strict_plain_name_body_char(const char character) -> bool {
    return (character >= 'A' && character <= 'Z') ||
           (character >= 'a' && character <= 'z') ||
           (character >= '0' && character <= '9') || character == '_' ||
           character == ':' || character == '.' || character == '-';
  }

  static auto is_strict_plain_name(const std::string_view fragment) -> bool {
    if (fragment.empty()) {
      return false;
    }
    if (!is_strict_plain_name_first_char(fragment.front())) {
      return false;
    }
    for (std::size_t index{1}; index < fragment.size(); ++index) {
      if (!is_strict_plain_name_body_char(fragment[index])) {
        return false;
      }
    }
    return true;
  }

  static auto sanitize_anchor_name(const std::string_view original,
                                   const std::set<std::string> &existing_names)
      -> std::string {
    static const AnchorCharPolicy POLICY{
        .is_valid_first = &is_strict_plain_name_first_char,
        .is_valid_body = &is_strict_plain_name_body_char};
    return sanitize_anchor_with_policy(original, existing_names, POLICY);
  }

  static auto extract_id_fragment(const sourcemeta::core::JSON &id_value)
      -> std::optional<std::string> {
    if (!id_value.is_string()) {
      return std::nullopt;
    }
    const sourcemeta::core::URI uri{id_value.to_string()};
    const auto fragment{uri.fragment()};
    if (!fragment.has_value()) {
      return std::nullopt;
    }
    return std::string{fragment.value()};
  }

  static auto
  subschema_id_fragment_is_invalid(const sourcemeta::core::JSON &subschema)
      -> bool {
    if (!subschema.is_object() || !subschema.defines("id") ||
        !subschema.at("id").is_string()) {
      return false;
    }
    const auto fragment{extract_id_fragment(subschema.at("id"))};
    if (!fragment.has_value() || fragment.value().empty()) {
      return false;
    }
    return !is_strict_plain_name(fragment.value());
  }

  static auto
  subschema_starts_sub_resource(const sourcemeta::core::JSON &subschema)
      -> bool {
    if (!subschema.is_object() || !subschema.defines("id") ||
        !subschema.at("id").is_string()) {
      return false;
    }
    const sourcemeta::core::URI uri{subschema.at("id").to_string()};
    if (uri.is_fragment_only()) {
      return false;
    }
    const auto without_fragment{uri.recompose_without_fragment()};
    return without_fragment.has_value() && !without_fragment.value().empty();
  }

  static auto collect_resource_anchors(const sourcemeta::core::JSON &subschema,
                                       const bool is_root,
                                       std::set<std::string> &result) -> void {
    if (!subschema.is_object()) {
      return;
    }

    if (!is_root && subschema_starts_sub_resource(subschema)) {
      return;
    }

    if (subschema.defines("id") && subschema.at("id").is_string()) {
      const auto fragment{extract_id_fragment(subschema.at("id"))};
      if (fragment.has_value() && !fragment.value().empty()) {
        result.insert(fragment.value());
      }
    }

    for (const std::string_view object_keyword :
         {"definitions", "properties", "patternProperties", "dependencies"}) {
      if (subschema.defines(std::string{object_keyword}) &&
          subschema.at(std::string{object_keyword}).is_object()) {
        for (const auto &entry :
             subschema.at(std::string{object_keyword}).as_object()) {
          collect_resource_anchors(entry.second, false, result);
        }
      }
    }

    for (const std::string_view array_keyword : {"allOf", "anyOf", "oneOf"}) {
      if (subschema.defines(std::string{array_keyword}) &&
          subschema.at(std::string{array_keyword}).is_array()) {
        for (const auto &item :
             subschema.at(std::string{array_keyword}).as_array()) {
          collect_resource_anchors(item, false, result);
        }
      }
    }

    for (const std::string_view single_keyword :
         {"additionalProperties", "additionalItems", "not"}) {
      if (subschema.defines(std::string{single_keyword})) {
        collect_resource_anchors(subschema.at(std::string{single_keyword}),
                                 false, result);
      }
    }

    if (subschema.defines("items")) {
      const auto &items{subschema.at("items")};
      if (items.is_array()) {
        for (const auto &item : items.as_array()) {
          collect_resource_anchors(item, false, result);
        }
      } else {
        collect_resource_anchors(items, false, result);
      }
    }
  }

  static auto collect_invalid_anchors(const sourcemeta::core::JSON &subschema,
                                      const bool is_root,
                                      std::vector<std::string> &result)
      -> void {
    if (!subschema.is_object()) {
      return;
    }

    if (!is_root && subschema_starts_sub_resource(subschema)) {
      return;
    }

    if (subschema_id_fragment_is_invalid(subschema)) {
      const auto fragment{extract_id_fragment(subschema.at("id"))};
      result.push_back(fragment.value());
    }

    for (const std::string_view object_keyword :
         {"definitions", "properties", "patternProperties", "dependencies"}) {
      if (subschema.defines(std::string{object_keyword}) &&
          subschema.at(std::string{object_keyword}).is_object()) {
        for (const auto &entry :
             subschema.at(std::string{object_keyword}).as_object()) {
          collect_invalid_anchors(entry.second, false, result);
        }
      }
    }

    for (const std::string_view array_keyword : {"allOf", "anyOf", "oneOf"}) {
      if (subschema.defines(std::string{array_keyword}) &&
          subschema.at(std::string{array_keyword}).is_array()) {
        for (const auto &item :
             subschema.at(std::string{array_keyword}).as_array()) {
          collect_invalid_anchors(item, false, result);
        }
      }
    }

    for (const std::string_view single_keyword :
         {"additionalProperties", "additionalItems", "not"}) {
      if (subschema.defines(std::string{single_keyword})) {
        collect_invalid_anchors(subschema.at(std::string{single_keyword}),
                                false, result);
      }
    }

    if (subschema.defines("items")) {
      const auto &items{subschema.at("items")};
      if (items.is_array()) {
        for (const auto &item : items.as_array()) {
          collect_invalid_anchors(item, false, result);
        }
      } else {
        collect_invalid_anchors(items, false, result);
      }
    }
  }

  static auto
  build_resource_rename_map(const sourcemeta::core::JSON &resource_root)
      -> std::map<std::string, std::string> {
    std::set<std::string> existing;
    collect_resource_anchors(resource_root, true, existing);

    std::vector<std::string> invalid;
    collect_invalid_anchors(resource_root, true, invalid);

    std::map<std::string, std::string> renames;
    std::set<std::string> in_use{existing};
    for (const auto &original : invalid) {
      if (renames.contains(original)) {
        continue;
      }
      in_use.erase(original);
      const auto sanitized{sanitize_anchor_name(original, in_use)};
      renames.emplace(original, sanitized);
      in_use.insert(sanitized);
    }
    return renames;
  }

  static auto resource_has_descendant_with_pending_pattern(
      const sourcemeta::core::JSON &subschema, const bool is_root) -> bool {
    if (!subschema.is_object()) {
      return false;
    }
    if (!is_root && subschema_starts_sub_resource(subschema)) {
      return false;
    }
    if (!is_root && has_pending_draft_4_pattern(subschema)) {
      return true;
    }

    for (const std::string_view object_keyword :
         {"definitions", "properties", "patternProperties", "dependencies"}) {
      if (subschema.defines(std::string{object_keyword}) &&
          subschema.at(std::string{object_keyword}).is_object()) {
        for (const auto &entry :
             subschema.at(std::string{object_keyword}).as_object()) {
          if (resource_has_descendant_with_pending_pattern(entry.second,
                                                           false)) {
            return true;
          }
        }
      }
    }

    for (const std::string_view array_keyword : {"allOf", "anyOf", "oneOf"}) {
      if (subschema.defines(std::string{array_keyword}) &&
          subschema.at(std::string{array_keyword}).is_array()) {
        for (const auto &item :
             subschema.at(std::string{array_keyword}).as_array()) {
          if (resource_has_descendant_with_pending_pattern(item, false)) {
            return true;
          }
        }
      }
    }

    for (const std::string_view single_keyword :
         {"additionalProperties", "additionalItems", "not"}) {
      if (subschema.defines(std::string{single_keyword})) {
        if (resource_has_descendant_with_pending_pattern(
                subschema.at(std::string{single_keyword}), false)) {
          return true;
        }
      }
    }

    if (subschema.defines("items")) {
      const auto &items{subschema.at("items")};
      if (items.is_array()) {
        for (const auto &item : items.as_array()) {
          if (resource_has_descendant_with_pending_pattern(item, false)) {
            return true;
          }
        }
      } else if (resource_has_descendant_with_pending_pattern(items, false)) {
        return true;
      }
    }

    return false;
  }

  static auto apply_anchor_renames_in_resource(
      sourcemeta::core::JSON &subschema, const bool is_root,
      const std::map<std::string, std::string> &renames,
      const std::optional<std::string> &resource_base) -> void {
    if (!subschema.is_object()) {
      return;
    }

    if (!is_root && subschema_starts_sub_resource(subschema)) {
      return;
    }

    if (subschema.defines("id") && subschema.at("id").is_string()) {
      const auto &id_string{subschema.at("id").to_string()};
      const sourcemeta::core::URI uri{id_string};
      const auto fragment{uri.fragment()};
      if (fragment.has_value() && !fragment.value().empty()) {
        const auto rename_iter{renames.find(std::string{fragment.value()})};
        if (rename_iter != renames.end()) {
          if (uri.is_fragment_only()) {
            subschema.assign("id",
                             sourcemeta::core::JSON{"#" + rename_iter->second});
          } else {
            const auto without_fragment{uri.recompose_without_fragment()};
            subschema.assign(
                "id", sourcemeta::core::JSON{(without_fragment.has_value()
                                                  ? without_fragment.value()
                                                  : std::string{}) +
                                             "#" + rename_iter->second});
          }
        }
      }
    }

    if (subschema.defines("$ref") && subschema.at("$ref").is_string()) {
      const auto &ref_string{subschema.at("$ref").to_string()};
      const sourcemeta::core::URI ref_uri{ref_string};
      const auto fragment{ref_uri.fragment()};
      if (fragment.has_value() &&
          renames.contains(std::string{fragment.value()})) {
        const auto without_fragment{ref_uri.recompose_without_fragment()};
        const bool same_base =
            ref_uri.is_fragment_only() ||
            (resource_base.has_value() && without_fragment.has_value() &&
             without_fragment.value() == resource_base.value());
        if (same_base) {
          const auto &new_name{renames.at(std::string{fragment.value()})};
          subschema.assign(
              "$ref", sourcemeta::core::JSON{
                          ref_uri.is_fragment_only()
                              ? "#" + new_name
                              : (without_fragment.value() + "#" + new_name)});
        }
      }
    }

    for (const std::string_view object_keyword :
         {"definitions", "properties", "patternProperties", "dependencies"}) {
      if (subschema.defines(std::string{object_keyword}) &&
          subschema.at(std::string{object_keyword}).is_object()) {
        std::vector<std::string> keys;
        keys.reserve(subschema.at(std::string{object_keyword}).size());
        for (const auto &entry :
             subschema.at(std::string{object_keyword}).as_object()) {
          keys.push_back(entry.first);
        }
        for (const auto &key : keys) {
          apply_anchor_renames_in_resource(
              subschema.at(std::string{object_keyword}).at(key), false, renames,
              resource_base);
        }
      }
    }

    for (const std::string_view array_keyword : {"allOf", "anyOf", "oneOf"}) {
      if (subschema.defines(std::string{array_keyword}) &&
          subschema.at(std::string{array_keyword}).is_array()) {
        auto &array_value{subschema.at(std::string{array_keyword})};
        for (std::size_t index{0}; index < array_value.size(); ++index) {
          apply_anchor_renames_in_resource(array_value.at(index), false,
                                           renames, resource_base);
        }
      }
    }

    for (const std::string_view single_keyword :
         {"additionalProperties", "additionalItems", "not"}) {
      if (subschema.defines(std::string{single_keyword})) {
        apply_anchor_renames_in_resource(
            subschema.at(std::string{single_keyword}), false, renames,
            resource_base);
      }
    }

    if (subschema.defines("items")) {
      auto &items{subschema.at("items")};
      if (items.is_array()) {
        for (std::size_t index{0}; index < items.size(); ++index) {
          apply_anchor_renames_in_resource(items.at(index), false, renames,
                                           resource_base);
        }
      } else {
        apply_anchor_renames_in_resource(items, false, renames, resource_base);
      }
    }
  }

  static auto resource_needs_anchor_sanitization(
      const sourcemeta::core::JSON &resource_root) -> bool {
    std::vector<std::string> invalid;
    collect_invalid_anchors(resource_root, true, invalid);
    return !invalid.empty();
  }

  static auto enclosing_resource_has_pending_sanitization(
      const sourcemeta::core::SchemaFrame::Location &location,
      const sourcemeta::core::JSON &root,
      const sourcemeta::core::SchemaFrame &frame) -> bool {
    std::optional<sourcemeta::core::WeakPointer> closest;
    for (const auto &entry : frame.locations()) {
      const bool entry_is_resource_scope =
          entry.second.type ==
              sourcemeta::core::SchemaFrame::LocationType::Resource ||
          entry.second.pointer.empty();
      if (!entry_is_resource_scope) {
        continue;
      }
      if (entry.second.pointer.size() > location.pointer.size()) {
        continue;
      }
      bool is_ancestor{true};
      for (std::size_t index{0}; index < entry.second.pointer.size(); ++index) {
        if (!(entry.second.pointer.at(index) == location.pointer.at(index))) {
          is_ancestor = false;
          break;
        }
      }
      if (!is_ancestor) {
        continue;
      }
      if (!closest.has_value() ||
          entry.second.pointer.size() > closest.value().size()) {
        closest = entry.second.pointer;
      }
    }
    if (!closest.has_value()) {
      return false;
    }
    const auto closest_pointer{sourcemeta::core::to_pointer(closest.value())};
    const auto &resource_schema{sourcemeta::core::get(root, closest_pointer)};
    return resource_needs_anchor_sanitization(resource_schema);
  }
};
