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
            const SchemaWalker &, const SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
                          Vocabularies::Known::JSON_Schema_2019_09_Applicator,
                          Vocabularies::Known::JSON_Schema_Draft_7}) &&
                     schema.is_object());

    const auto *else_value{schema.try_at(KEYWORD)};
    ONLY_CONTINUE_IF(else_value && is_schema(*else_value) &&
                     is_empty_schema(*else_value));
    const auto *if_value{schema.try_at("if")};
    ONLY_CONTINUE_IF(else_value->is_object() || !if_value ||
                     !(if_value->is_boolean() && if_value->to_boolean()));
    ONLY_CONTINUE_IF(!frame.has_references_through(
        location.pointer, WeakPointer::Token{std::cref(KEYWORD)}));
    return APPLIES_TO_KEYWORDS(KEYWORD);
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase(KEYWORD);
  }
};
