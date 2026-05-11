class UpgradeDraft3ToDraft4 final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  UpgradeDraft3ToDraft4()
      : SchemaTransformRule{"upgrade_draft_3_to_draft_4", ""} {};

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
        vocabularies.contains(Vocabularies::Known::JSON_Schema_Draft_3) &&
        schema.is_object());

    const bool root_via_default_dialect =
        location.pointer.empty() && !schema.defines("$schema");

    ONLY_CONTINUE_IF(has_pending_draft_3_pattern(schema) ||
                     root_via_default_dialect);

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

      if (has_pending_draft_3_pattern(entry_schema)) {
        return false;
      }
    }

    return true;
  }

  auto transform(sourcemeta::core::JSON &schema, const Result &) const
      -> void override {
    rewrite_type_any(schema);
    rewrite_type_array_with_subschemas(schema);
    rewrite_disallow(schema);
    rewrite_extends(schema);
    rewrite_divisible_by(schema);
    rewrite_required_property_booleans(schema);
    rewrite_dependencies_string_form(schema);
    rewrite_format(schema);

    if (schema.defines("$schema") && schema.at("$schema").is_string() &&
        schema.at("$schema").to_string() == DRAFT_3_URL) {
      schema.assign("$schema", sourcemeta::core::JSON{DRAFT_4_URL});
      drop_dialect_overrides(schema, true);
    } else {
      mark_dialect_override(schema, DRAFT_4_URL);
    }
  }

private:
  static inline const std::string DRAFT_3_URL{
      "http://json-schema.org/draft-03/schema#"};
  static inline const std::string DRAFT_4_URL{
      "http://json-schema.org/draft-04/schema#"};

  static auto
  has_pending_draft_3_pattern(const sourcemeta::core::JSON &subschema) -> bool {
    if (!subschema.is_object()) {
      return false;
    }

    if (subschema.defines("$schema") && subschema.at("$schema").is_string() &&
        subschema.at("$schema").to_string() == DRAFT_3_URL) {
      return true;
    }

    const auto *type_value{subschema.try_at("type")};
    if (type_value != nullptr) {
      if (type_value->is_string() && type_value->to_string() == "any") {
        return true;
      }
      if (type_value->is_array()) {
        for (const auto &element : type_value->as_array()) {
          if (element.is_string() && element.to_string() == "any") {
            return true;
          }
          if (element.is_object()) {
            return true;
          }
        }
      }
    }

    const auto *disallow_value{subschema.try_at("disallow")};
    if (disallow_value != nullptr &&
        (disallow_value->is_string() || disallow_value->is_array() ||
         disallow_value->is_object())) {
      return true;
    }

    const auto *extends_value{subschema.try_at("extends")};
    if (extends_value != nullptr &&
        (extends_value->is_array() || extends_value->is_object())) {
      return true;
    }

    if (subschema.defines("divisibleBy")) {
      return true;
    }

    const auto *properties{subschema.try_at("properties")};
    if (properties != nullptr && properties->is_object()) {
      for (const auto &entry : properties->as_object()) {
        if (entry.second.is_object() && entry.second.defines("required") &&
            entry.second.at("required").is_boolean()) {
          return true;
        }
      }
    }

    const auto *dependencies{subschema.try_at("dependencies")};
    if (dependencies != nullptr && dependencies->is_object()) {
      for (const auto &entry : dependencies->as_object()) {
        if (entry.second.is_string()) {
          return true;
        }
      }
    }

    if (has_renamable_draft_3_format(subschema)) {
      return true;
    }

    return false;
  }

  static auto
  has_renamable_draft_3_format(const sourcemeta::core::JSON &subschema)
      -> bool {
    if (!subschema.is_object()) {
      return false;
    }
    const auto *format_value{subschema.try_at("format")};
    if (format_value == nullptr || !format_value->is_string()) {
      return false;
    }
    const auto &name{format_value->to_string()};
    return name == "host-name" || name == "ip-address";
  }

  static auto rewrite_type_any(sourcemeta::core::JSON &schema) -> void {
    if (!schema.defines("type")) {
      return;
    }
    auto &type_value{schema.at("type")};
    if (type_value.is_string()) {
      if (type_value.to_string() == "any") {
        schema.erase("type");
      }
      return;
    }
    if (!type_value.is_array()) {
      return;
    }
    bool collapses{false};
    for (const auto &element : type_value.as_array()) {
      if (element.is_string() && element.to_string() == "any") {
        collapses = true;
        break;
      }
    }
    if (collapses) {
      schema.erase("type");
    }
  }

  static auto rewrite_type_array_with_subschemas(sourcemeta::core::JSON &schema)
      -> void {
    if (!schema.defines("type")) {
      return;
    }
    auto &type_value{schema.at("type")};
    if (!type_value.is_array()) {
      return;
    }
    bool has_subschema{false};
    for (const auto &element : type_value.as_array()) {
      if (element.is_object()) {
        has_subschema = true;
        break;
      }
    }
    if (!has_subschema) {
      return;
    }

    auto branches{sourcemeta::core::JSON::make_array()};
    for (const auto &element : type_value.as_array()) {
      if (element.is_string()) {
        auto branch{sourcemeta::core::JSON::make_object()};
        branch.assign("type", element);
        branches.push_back(std::move(branch));
      } else if (element.is_object()) {
        branches.push_back(element);
      }
    }
    schema.erase("type");
    schema.assign("anyOf", std::move(branches));
  }

  static auto type_string_to_branch(const std::string &type_name)
      -> sourcemeta::core::JSON {
    auto branch{sourcemeta::core::JSON::make_object()};
    branch.assign("type", sourcemeta::core::JSON{type_name});
    return branch;
  }

  static auto rewrite_disallow(sourcemeta::core::JSON &schema) -> void {
    if (!schema.defines("disallow") || schema.defines("not")) {
      return;
    }

    const auto &disallow{schema.at("disallow")};
    if (!disallow.is_string() && !disallow.is_array() &&
        !disallow.is_object()) {
      return;
    }

    if (disallow.is_string() && disallow.to_string() == "any") {
      schema.erase("disallow");
      schema.assign("not", sourcemeta::core::JSON::make_object());
      return;
    }

    if (disallow.is_array()) {
      for (const auto &element : disallow.as_array()) {
        if (element.is_string() && element.to_string() == "any") {
          schema.erase("disallow");
          schema.assign("not", sourcemeta::core::JSON::make_object());
          return;
        }
      }
    }

    auto negated{sourcemeta::core::JSON::make_object()};
    if (disallow.is_string()) {
      negated.assign("type", disallow);
    } else if (disallow.is_array()) {
      bool has_subschema{false};
      for (const auto &element : disallow.as_array()) {
        if (element.is_object()) {
          has_subschema = true;
          break;
        }
      }
      if (!has_subschema) {
        negated.assign("type", disallow);
      } else {
        auto branches{sourcemeta::core::JSON::make_array()};
        for (const auto &element : disallow.as_array()) {
          if (element.is_string()) {
            branches.push_back(type_string_to_branch(element.to_string()));
          } else if (element.is_object()) {
            branches.push_back(element);
          }
        }
        negated.assign("anyOf", std::move(branches));
      }
    } else {
      negated = disallow;
    }

    schema.erase("disallow");
    schema.assign("not", std::move(negated));
  }

  static auto rewrite_extends(sourcemeta::core::JSON &schema) -> void {
    if (!schema.defines("extends") || schema.defines("allOf")) {
      return;
    }

    const auto &extends{schema.at("extends")};
    if (!extends.is_array() && !extends.is_object()) {
      return;
    }

    auto value{extends};
    schema.erase("extends");

    if (value.is_array()) {
      schema.assign("allOf", std::move(value));
      return;
    }

    auto array{sourcemeta::core::JSON::make_array()};
    array.push_back(std::move(value));
    schema.assign("allOf", std::move(array));
  }

  static auto rewrite_divisible_by(sourcemeta::core::JSON &schema) -> void {
    if (!schema.defines("divisibleBy") || schema.defines("multipleOf")) {
      return;
    }
    schema.rename("divisibleBy", "multipleOf");
  }

  static auto rewrite_required_property_booleans(sourcemeta::core::JSON &schema)
      -> void {
    if (!schema.defines("properties") || !schema.at("properties").is_object()) {
      return;
    }

    std::vector<std::string> newly_required;
    auto &properties{schema.at("properties")};
    std::vector<std::string> property_keys;
    property_keys.reserve(properties.size());
    for (const auto &entry : properties.as_object()) {
      property_keys.push_back(entry.first);
    }

    for (const auto &key : property_keys) {
      auto &property{properties.at(key)};
      if (!property.is_object() || !property.defines("required")) {
        continue;
      }
      const auto &required_value{property.at("required")};
      if (!required_value.is_boolean()) {
        continue;
      }
      const bool is_required{required_value.to_boolean()};
      property.erase("required");
      if (is_required) {
        newly_required.push_back(key);
      }
    }

    if (newly_required.empty()) {
      return;
    }

    if (!schema.defines("required") || !schema.at("required").is_array()) {
      auto fresh{sourcemeta::core::JSON::make_array()};
      for (const auto &name : newly_required) {
        fresh.push_back(sourcemeta::core::JSON{name});
      }
      schema.try_assign_before("required", fresh, "properties");
      return;
    }

    auto &existing{schema.at("required")};
    std::set<std::string> already;
    for (const auto &item : existing.as_array()) {
      if (item.is_string()) {
        already.insert(item.to_string());
      }
    }
    for (const auto &name : newly_required) {
      if (already.contains(name)) {
        continue;
      }
      existing.push_back(sourcemeta::core::JSON{name});
      already.insert(name);
    }
  }

  static auto rewrite_dependencies_string_form(sourcemeta::core::JSON &schema)
      -> void {
    if (!schema.defines("dependencies") ||
        !schema.at("dependencies").is_object()) {
      return;
    }
    auto &dependencies{schema.at("dependencies")};
    std::vector<std::string> string_keys;
    for (const auto &entry : dependencies.as_object()) {
      if (entry.second.is_string()) {
        string_keys.push_back(entry.first);
      }
    }
    for (const auto &key : string_keys) {
      const auto value{dependencies.at(key).to_string()};
      auto array{sourcemeta::core::JSON::make_array()};
      array.push_back(sourcemeta::core::JSON{value});
      dependencies.assign(key, std::move(array));
    }
  }

  static auto rewrite_format(sourcemeta::core::JSON &schema) -> void {
    if (!schema.defines("format")) {
      return;
    }
    auto &format_value{schema.at("format")};
    if (!format_value.is_string()) {
      return;
    }
    const auto &name{format_value.to_string()};
    if (name == "host-name") {
      schema.assign("format", sourcemeta::core::JSON{"hostname"});
    } else if (name == "ip-address") {
      schema.assign("format", sourcemeta::core::JSON{"ipv4"});
    }
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
};
