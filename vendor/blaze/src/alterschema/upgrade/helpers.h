static constexpr std::string_view DIALECT_OVERRIDE_KEYWORD{
    "x-sourcemeta-dialect-override-subschema"};

static auto mark_dialect_override(sourcemeta::core::JSON &schema,
                                  const std::string_view dialect) -> void {
  if (!schema.defines(std::string{DIALECT_OVERRIDE_KEYWORD})) {
    schema.assign(std::string{DIALECT_OVERRIDE_KEYWORD},
                  sourcemeta::core::JSON{std::string{dialect}});
  }
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

  schema.erase(std::string{DIALECT_OVERRIDE_KEYWORD});

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
