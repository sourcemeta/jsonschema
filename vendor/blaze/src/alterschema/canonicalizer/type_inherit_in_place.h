class TypeInheritInPlace final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  TypeInheritInPlace()
      : SchemaTransformRule{
            "type_inherit_in_place",
            "An untyped schema inside an in-place applicator inherits "
            "the type from its nearest typed ancestor"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &root,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &walker,
            const sourcemeta::core::SchemaResolver &resolver) const
      -> SchemaTransformRule::Result override {
    using namespace sourcemeta::core;
    ONLY_CONTINUE_IF(schema.is_object());
    ONLY_CONTINUE_IF(vocabularies.contains_any(
        {Vocabularies::Known::JSON_Schema_2020_12_Validation,
         Vocabularies::Known::JSON_Schema_2019_09_Validation,
         Vocabularies::Known::JSON_Schema_Draft_7,
         Vocabularies::Known::JSON_Schema_Draft_6,
         Vocabularies::Known::JSON_Schema_Draft_4,
         Vocabularies::Known::JSON_Schema_Draft_3,
         Vocabularies::Known::JSON_Schema_Draft_2,
         Vocabularies::Known::JSON_Schema_Draft_1,
         Vocabularies::Known::JSON_Schema_Draft_0}));
    ONLY_CONTINUE_IF(!schema.defines("type"));
    ONLY_CONTINUE_IF(!schema.defines("enum"));
    ONLY_CONTINUE_IF(!vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Validation,
                          Vocabularies::Known::JSON_Schema_2019_09_Validation,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6}) ||
                     !schema.defines("const"));

    for (const auto &entry : schema.as_object()) {
      const auto &keyword_type{walker(entry.first, vocabularies).type};
      ONLY_CONTINUE_IF(keyword_type != SchemaKeywordType::Reference);
      ONLY_CONTINUE_IF(keyword_type ==
                           SchemaKeywordType::ApplicatorValueInPlaceOther ||
                       !IS_IN_PLACE_APPLICATOR(keyword_type));
    }

    // Walk up through in-place applicators excluding `allOf`. In `allOf` the
    // parent's type already constrains all branches (a conjunction), and other
    // rules may want to lift type out of conjunctions
    const auto ancestor{WALK_UP(
        root, frame, location, walker, resolver,
        [](const SchemaKeywordType keyword_type) {
          return IS_IN_PLACE_APPLICATOR(keyword_type) &&
                 keyword_type != SchemaKeywordType::ApplicatorElementsInPlace;
        },
        [](const JSON &ancestor_schema, const Vocabularies &) {
          return ancestor_schema.defines("type");
        })};

    ONLY_CONTINUE_IF(ancestor.has_value());
    this->inherited_type_ = get(root, ancestor.value().get()).at("type");
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.assign("type", this->inherited_type_);
  }

private:
  mutable sourcemeta::core::JSON inherited_type_{
      sourcemeta::core::JSON{nullptr}};
};
