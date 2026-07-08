class DefinitionsToDefs final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  DefinitionsToDefs() : SchemaTransformRule{"definitions_to_defs"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Core,
                          Vocabularies::Known::JSON_Schema_2019_09_Core}) &&
                     schema.is_object() && schema.defines("definitions") &&
                     !schema.defines("$defs"));
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.rename("definitions", "$defs");
  }

  [[nodiscard]] auto rereference(const std::string_view,
                                 const sourcemeta::core::Pointer &,
                                 const sourcemeta::core::Pointer &target,
                                 const sourcemeta::core::Pointer &current) const
      -> std::optional<sourcemeta::core::Pointer> override {
    return target.rebase(current.concat("definitions"),
                         current.concat("$defs"));
  }
};
