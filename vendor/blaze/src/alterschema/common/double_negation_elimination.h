class DoubleNegationElimination final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  DoubleNegationElimination()
      : SchemaTransformRule{
            "double_negation_elimination",
            "A `not` whose value is a schema containing only another "
            "`not` is equivalent to the inner value"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    static const JSON::String KEYWORD{"not"};
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
                          Vocabularies::Known::JSON_Schema_2019_09_Applicator,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4}) &&
                     schema.is_object() && schema.defines(KEYWORD) &&
                     schema.at(KEYWORD).is_object() &&
                     schema.at(KEYWORD).size() == 1 &&
                     schema.at(KEYWORD).defines(KEYWORD) &&
                     !(schema.at(KEYWORD).at(KEYWORD).is_boolean() &&
                       !schema.at(KEYWORD).at(KEYWORD).to_boolean()));
    ONLY_CONTINUE_IF(
        !(vocabularies.contains_any(
              {Vocabularies::Known::JSON_Schema_2020_12_Unevaluated,
               Vocabularies::Known::JSON_Schema_2019_09_Applicator}) &&
          (schema.defines("unevaluatedProperties") ||
           schema.defines("unevaluatedItems"))));
    ONLY_CONTINUE_IF(!frame.has_references_through(
        location.pointer, WeakPointer::Token{std::cref(KEYWORD)}));
    return APPLIES_TO_KEYWORDS(KEYWORD);
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    auto inner{schema.at("not").at("not")};
    schema.erase("not");

    while (inner.is_object() && inner.size() == 1 && inner.defines("not") &&
           inner.at("not").is_object() && inner.at("not").size() == 1 &&
           inner.at("not").defines("not") &&
           !(inner.at("not").at("not").is_boolean() &&
             !inner.at("not").at("not").to_boolean())) {
      auto next{inner.at("not").at("not")};
      inner = std::move(next);
    }

    if (inner.is_object()) {
      schema.merge(inner.as_object());
    }
  }

  [[nodiscard]] auto rereference(const std::string_view, const Pointer &,
                                 const Pointer &target,
                                 const Pointer &current) const
      -> Pointer override {
    auto old_prefix{current.concat({"not", "not"})};
    while (target.starts_with(old_prefix.concat({"not", "not"}))) {
      old_prefix = old_prefix.concat({"not", "not"});
    }
    if (!target.starts_with(old_prefix)) {
      return target;
    }
    return target.rebase(old_prefix, current);
  }
};
