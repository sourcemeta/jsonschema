class ShadowStrayDialectDeclaration final : public SchemaTransformRule {
public:
  ShadowStrayDialectDeclaration()
      : SchemaTransformRule{"shadow_stray_dialect_declaration"} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const sourcemeta::core::JSON &,
                               const sourcemeta::core::SchemaVocabularies &,
                               const Site &location,
                               const sourcemeta::core::SchemaWalker &,
                               const sourcemeta::core::SchemaResolver &) const
      -> bool override {
    ONLY_CONTINUE_IF(schema.is_object() && schema.defines("$schema") &&
                     schema.at("$schema").is_string());

    // A dialect is declared where a resource begins and nowhere else, which
    // is what both Draft 7 core 7 and 2019-09 core 8.1.1 say and what framing
    // implements. A `$schema` anywhere else is the author's data, and reading
    // it as a dialect is what leaves this module and framing disagreeing about
    // what dialect a subschema is on: the rules wait for it to move while
    // framing never put it there
    // A dialect may be declared where a resource begins, which is where the
    // subschema's own pointer and the offset of its resource meet. Asking the
    // location's type instead does not work here: the driver walks subschema
    // locations, and a resource root has one of those too
    ONLY_CONTINUE_IF(!location.pointer.empty() &&
                     location.pointer.size() != location.relative_pointer);

    // The ladder's own marker may sit beside this `$schema`, and it is left
    // alone: it records the dialect reached so far, which renaming a
    // declaration that declares nothing does not disturb. Standing down while
    // one is present would mean shadowing the same `$schema` only once the
    // marker had gone, so converting a document twice would not agree with
    // converting it once

    return true;
  }

  auto transform(sourcemeta::core::JSON &schema, const Site &) const
      -> void override {
    std::string shadowed{"x-$schema"};
    while (schema.defines(shadowed)) {
      shadowed.insert(0, "x-");
    }

    schema.rename("$schema", std::move(shadowed));
  }
};
