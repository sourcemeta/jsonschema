/// Core reads `x-sourcemeta-dialect-override-subschema` as a dialect, ahead of
/// any `$schema` beside it, so a document carrying one has whatever dialect it
/// names whether or not the author meant that. Settling it into a real
/// `$schema` is what makes the rest of the conversion, and the output, depend
/// on nothing but what JSON Schema itself defines
class DialectOverrideBecomesDollarSchema final : public SchemaTransformRule {
public:
  DialectOverrideBecomesDollarSchema()
      : SchemaTransformRule{"dialect_override_becomes_dollar_schema"} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const sourcemeta::core::JSON &,
                               const sourcemeta::core::SchemaVocabularies &,
                               const Site &,
                               const sourcemeta::core::SchemaWalker &,
                               const sourcemeta::core::SchemaResolver &) const
      -> bool override {
    if (!schema.is_object()) {
      return false;
    }

    // Only a value naming a dialect the ladder walks through is one framing
    // would read as a dialect here. Any other value is the author's own data
    // that happens to share the name, and is left exactly as written
    const auto *value{schema.try_at(DIALECT_OVERRIDE_KEYWORD)};
    return value != nullptr && is_own_dialect_override(*value);
  }

  auto transform(sourcemeta::core::JSON &schema, const Site &site) const
      -> void override {
    auto dialect{schema.at(DIALECT_OVERRIDE_KEYWORD)};
    schema.erase(DIALECT_OVERRIDE_KEYWORD);

    // Only where framing would read a `$schema` does one have to be written to
    // keep the dialect the document had. Anywhere else the keyword said
    // nothing that a `$schema` could say, and the subschema takes the dialect
    // of the resource holding it
    if (!dialect.is_string() || schema.defines("$schema") ||
        !at_dialect_declaration(schema, site)) {
      return;
    }

    assign_before_first_key(schema, "$schema", std::move(dialect));
  }
};
