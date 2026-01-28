class OneOfToAnyOfDisjointTypes final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  OneOfToAnyOfDisjointTypes()
      : SchemaTransformRule{
            "oneof_to_anyof_disjoint_types",
            "A `oneOf` where all branches have disjoint types can be safely "
            "converted to `anyOf`"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    static const JSON::String KEYWORD{"oneOf"};
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
                          Vocabularies::Known::JSON_Schema_2019_09_Applicator,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4}) &&
                     schema.is_object() && schema.defines(KEYWORD) &&
                     schema.at(KEYWORD).is_array() &&
                     schema.at(KEYWORD).size() > 1);

    const auto has_validation_vocabulary{vocabularies.contains_any(
        {Vocabularies::Known::JSON_Schema_2020_12_Validation,
         Vocabularies::Known::JSON_Schema_2019_09_Validation,
         Vocabularies::Known::JSON_Schema_Draft_7,
         Vocabularies::Known::JSON_Schema_Draft_6,
         Vocabularies::Known::JSON_Schema_Draft_4,
         Vocabularies::Known::JSON_Schema_Draft_3,
         Vocabularies::Known::JSON_Schema_Draft_2,
         Vocabularies::Known::JSON_Schema_Draft_1})};

    const auto has_const_vocabulary{vocabularies.contains_any(
        {Vocabularies::Known::JSON_Schema_2020_12_Validation,
         Vocabularies::Known::JSON_Schema_2019_09_Validation,
         Vocabularies::Known::JSON_Schema_Draft_7,
         Vocabularies::Known::JSON_Schema_Draft_6})};

    const auto &oneof{schema.at(KEYWORD)};
    std::vector<JSON::TypeSet> type_sets;
    type_sets.reserve(oneof.size());

    for (const auto &branch : oneof.as_array()) {
      ONLY_CONTINUE_IF(branch.is_object());

      const auto has_type{branch.defines("type")};
      const auto has_const{has_const_vocabulary && branch.defines("const")};
      const auto has_enum{has_validation_vocabulary && branch.defines("enum") &&
                          branch.at("enum").is_array()};

      if (has_type) {
        type_sets.push_back(parse_schema_type(branch.at("type")));
      } else if (has_const && !has_enum) {
        JSON::TypeSet branch_types;
        branch_types.set(static_cast<std::size_t>(branch.at("const").type()));
        type_sets.push_back(branch_types);
      } else if (has_enum && !has_const) {
        JSON::TypeSet branch_types;
        for (const auto &item : branch.at("enum").as_array()) {
          branch_types.set(static_cast<std::size_t>(item.type()));
        }
        type_sets.push_back(branch_types);
      } else {
        return false;
      }
    }

    for (std::size_t index = 0; index < type_sets.size(); ++index) {
      for (std::size_t other = index + 1; other < type_sets.size(); ++other) {
        ONLY_CONTINUE_IF((type_sets[index] & type_sets[other]).none());
      }
    }

    return APPLIES_TO_KEYWORDS(KEYWORD);
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.rename("oneOf", "anyOf");
  }

  [[nodiscard]] auto
  rereference(const std::string_view, const Pointer &origin [[maybe_unused]],
              const Pointer &target, const Pointer &current) const
      -> Pointer override {
    const Pointer oneof_prefix{current.concat({"oneOf"})};
    const Pointer anyof_prefix{current.concat({"anyOf"})};
    return target.rebase(oneof_prefix, anyof_prefix);
  }
};
