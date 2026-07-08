class DoubleNegationElimination final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  DoubleNegationElimination()
      : SchemaTransformRule{"double_negation_elimination"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &frame,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    static const sourcemeta::core::JSON::String KEYWORD{"not"};
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
                          Vocabularies::Known::JSON_Schema_2019_09_Applicator,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4}) &&
                     schema.is_object());

    const auto *outer_not{schema.try_at(KEYWORD)};
    ONLY_CONTINUE_IF(outer_not && outer_not->is_object() &&
                     outer_not->size() == 1);
    const auto *inner_not{outer_not->try_at(KEYWORD)};
    ONLY_CONTINUE_IF(inner_not &&
                     !(inner_not->is_boolean() && !inner_not->to_boolean()));
    ONLY_CONTINUE_IF(
        !(vocabularies.contains_any(
              {Vocabularies::Known::JSON_Schema_2020_12_Unevaluated,
               Vocabularies::Known::JSON_Schema_2019_09_Applicator}) &&
          (schema.defines("unevaluatedProperties") ||
           schema.defines("unevaluatedItems"))));
    ONLY_CONTINUE_IF(!frame.has_references_through(
        location.pointer,
        sourcemeta::core::WeakPointer::Token{std::cref(KEYWORD)}));
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
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

  [[nodiscard]] auto rereference(const std::string_view,
                                 const sourcemeta::core::Pointer &,
                                 const sourcemeta::core::Pointer &target,
                                 const sourcemeta::core::Pointer &current) const
      -> std::optional<sourcemeta::core::Pointer> override {
    auto old_prefix{current.concat(sourcemeta::core::Pointer{"not", "not"})};
    while (target.starts_with(
        old_prefix.concat(sourcemeta::core::Pointer{"not", "not"}))) {
      old_prefix = old_prefix.concat(sourcemeta::core::Pointer{"not", "not"});
    }
    if (!target.starts_with(old_prefix)) {
      return target;
    }
    return target.rebase(old_prefix, current);
  }
};
