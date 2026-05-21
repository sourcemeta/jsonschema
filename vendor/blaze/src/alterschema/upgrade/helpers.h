static const std::string DIALECT_OVERRIDE_KEYWORD{
    "x-sourcemeta-dialect-override-subschema"};

static auto mark_dialect_override(sourcemeta::core::JSON &schema,
                                  const std::string_view dialect) -> void {
  schema.assign(DIALECT_OVERRIDE_KEYWORD, sourcemeta::core::JSON{dialect});
}

static auto current_dialect_or_override(const sourcemeta::core::JSON &schema)
    -> std::string_view {
  if (!schema.is_object()) {
    return {};
  }
  const auto *override_value{schema.try_at(DIALECT_OVERRIDE_KEYWORD)};
  if (override_value != nullptr && override_value->is_string()) {
    return override_value->to_string();
  }
  if (schema.defines("$schema") && schema.at("$schema").is_string()) {
    return schema.at("$schema").to_string();
  }
  return {};
}

static auto
subschema_at_dialect(const sourcemeta::core::JSON &schema,
                     const sourcemeta::blaze::SchemaFrame::Location &location,
                     const std::string_view dialect) -> bool {
  const auto current{current_dialect_or_override(schema)};
  if (!current.empty()) {
    return current == dialect;
  }
  return schema.is_object() && location.pointer.empty();
}

static auto drop_dialect_overrides(sourcemeta::core::JSON &schema,
                                   const bool is_root) -> void {
  if (schema.is_array()) {
    for (auto &item : schema.as_array()) {
      drop_dialect_overrides(item, false);
    }
    return;
  }

  if (!schema.is_object()) {
    return;
  }

  if (!is_root && schema.defines("$schema") &&
      schema.at("$schema").is_string()) {
    return;
  }

  schema.erase(DIALECT_OVERRIDE_KEYWORD);

  std::vector<std::string> keys;
  keys.reserve(schema.size());
  for (const auto &entry : schema.as_object()) {
    keys.push_back(entry.first);
  }
  for (const auto &key : keys) {
    drop_dialect_overrides(schema.at(key), false);
  }
}

struct AnchorCharPolicy {
  std::function<bool(char)> is_valid_first;
  std::function<bool(char)> is_valid_body;
};

static auto sanitize_anchor_with_policy(const std::string_view original,
                                        const std::set<std::string> &in_use,
                                        const AnchorCharPolicy &policy)
    -> std::string {
  std::string sanitized;
  sanitized.reserve(original.size());
  for (const char character : original) {
    sanitized.push_back(policy.is_valid_body(character) ? character : '-');
  }
  while (sanitized.empty() || !policy.is_valid_first(sanitized.front()) ||
         in_use.contains(sanitized)) {
    sanitized.insert(0, "x-");
  }
  return sanitized;
}
