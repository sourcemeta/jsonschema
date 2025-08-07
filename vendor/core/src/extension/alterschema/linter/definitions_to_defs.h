class DefinitionsToDefs final : public SchemaTransformRule {
public:
  DefinitionsToDefs()
      : SchemaTransformRule{"definitions_to_defs",
                            "`definitions` was superseded by `$defs` in "
                            "2019-09 and later versions"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    return contains_any(vocabularies,
                        {"https://json-schema.org/draft/2020-12/vocab/core",
                         "https://json-schema.org/draft/2019-09/vocab/core"}) &&
           schema.is_object() && schema.defines("definitions") &&
           !schema.defines("$defs");
  }

  auto transform(JSON &schema) const -> void override {
    schema.rename("definitions", "$defs");
  }

  [[nodiscard]] auto rereference(const std::string &, const Pointer &,
                                 const Pointer &target,
                                 const Pointer &current) const
      -> Pointer override {
    return target.rebase(current.concat({"definitions"}),
                         current.concat({"$defs"}));
  }
};
