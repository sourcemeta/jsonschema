class ExtendsToArray final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  ExtendsToArray() : SchemaTransformRule{"extends_to_array", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any({Vocabularies::Known::JSON_Schema_Draft_0,
                                   Vocabularies::Known::JSON_Schema_Draft_1,
                                   Vocabularies::Known::JSON_Schema_Draft_2,
                                   Vocabularies::Known::JSON_Schema_Draft_3}) &&
        schema.is_object() && schema.defines("extends") &&
        !schema.at("extends").is_array());
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    auto array{sourcemeta::core::JSON::make_array()};
    array.push_back(schema.at("extends"));
    schema.assign("extends", std::move(array));
  }

  [[nodiscard]] auto rereference(const std::string_view, const Pointer &,
                                 const Pointer &target,
                                 const Pointer &current) const
      -> Pointer override {
    return target.rebase(current.concat({"extends"}),
                         current.concat({"extends", 0}));
  }
};
