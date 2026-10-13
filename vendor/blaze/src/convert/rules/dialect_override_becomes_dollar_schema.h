/// Core reads `x-sourcemeta-dialect-override-subschema` as a dialect, ahead of
/// any `$schema` beside it, so a document carrying one has whatever dialect it
/// names whether or not the author meant that. Settling it into a real
/// `$schema` is what makes the rest of the conversion, and the output, depend
/// on nothing but what JSON Schema itself defines.
///
/// That is only possible where a `$schema` is the thing framing reads. A marker
/// deeper than that names a dialect for one position, which JSON Schema has no
/// way to say, so it can only be settled when it says nothing: when it names
/// the dialect that position would have had anyway. One that names another
/// dialect stays exactly where it is, and `bump_dialect` moves it along with
/// the schema it governs, because dropping it would silently reinterpret every
/// keyword underneath it
class DialectOverrideBecomesDollarSchema final : public SchemaTransformRule {
public:
  DialectOverrideBecomesDollarSchema()
      : SchemaTransformRule{"dialect_override_becomes_dollar_schema"} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const sourcemeta::core::JSON &,
                               const sourcemeta::core::SchemaVocabularies &,
                               const Site &site,
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
    return value != nullptr && is_own_dialect_override(*value) &&
           (at_dialect_declaration(schema, site) || this->says_nothing(site));
  }

  auto begin_pass() const -> void override { this->redundant_.clear(); }

  auto plan(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const Site &site, const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> void override {
    this->redundant_.erase(site.pointer);
    if (!schema.is_object() || !location.parent.has_value()) {
      return;
    }

    const auto *value{schema.try_at(DIALECT_OVERRIDE_KEYWORD)};
    if (value == nullptr || !value->is_string()) {
      return;
    }

    // What the position would read as without the marker is what the resource
    // holding it reads as, which only the frame can say
    const auto enclosing{frame.traverse(location.parent.value())};
    if (enclosing.has_value() &&
        without_empty_fragment(enclosing.value().get().dialect) ==
            without_empty_fragment(value->to_string())) {
      this->redundant_.insert(site.pointer);
    }
  }

  auto transform(sourcemeta::core::JSON &schema, const Site &site) const
      -> void override {
    auto dialect{schema.at(DIALECT_OVERRIDE_KEYWORD)};
    schema.erase(DIALECT_OVERRIDE_KEYWORD);

    // Only where framing would read a `$schema` does one have to be written to
    // keep the dialect the document had. Anywhere else the marker said nothing
    // a `$schema` could say, and said nothing at all, so the subschema takes
    // the dialect of the resource holding it either way
    if (!dialect.is_string() || schema.defines("$schema") ||
        !at_dialect_declaration(schema, site)) {
      return;
    }

    assign_before_first_key(schema, "$schema", std::move(dialect));
  }

private:
  // Whether the marker at this position names the dialect the position would
  // read as without it
  [[nodiscard]] auto says_nothing(const Site &site) const -> bool {
    return this->redundant_.contains(site.pointer);
  }

  mutable std::set<sourcemeta::core::Pointer> redundant_;
};
