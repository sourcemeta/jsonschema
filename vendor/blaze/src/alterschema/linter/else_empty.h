class ElseEmpty final : public SchemaTransformRule {
private:
  static inline const std::string KEYWORD{"else"};

public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  ElseEmpty()
      : SchemaTransformRule{"else_empty",
                            "Setting the `else` keyword to the empty schema "
                            "does not add any further constraint"} {};

  [[nodiscard]] auto
  condition(const JSON &schema, const JSON &, const Vocabularies &vocabularies,
            const SchemaFrame &frame, const SchemaFrame::Location &location,
            const SchemaWalker &, const SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
             Vocabularies::Known::JSON_Schema_2019_09_Applicator,
             Vocabularies::Known::JSON_Schema_Draft_7}) &&
        schema.is_object() && schema.defines(KEYWORD) &&
        is_schema(schema.at(KEYWORD)) && is_empty_schema(schema.at(KEYWORD)) &&
        (schema.at(KEYWORD).is_object() ||
         (!schema.defines("if") ||
          !(schema.at("if").is_boolean() && schema.at("if").to_boolean()))));
    ONLY_CONTINUE_IF(!frame.has_references_through(
        location.pointer, WeakPointer::Token{std::cref(KEYWORD)}));
    return APPLIES_TO_KEYWORDS(KEYWORD);
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase(KEYWORD);
  }
};
