class OneOfToAnyOfDisjointTypes final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  OneOfToAnyOfDisjointTypes()
      : SchemaTransformRule{"oneof_to_anyof_disjoint_types"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    static const sourcemeta::core::JSON::String KEYWORD{"oneOf"};
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
                          Vocabularies::Known::JSON_Schema_2019_09_Applicator,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4}) &&
                     schema.is_object());

    const auto *oneof_value{schema.try_at(KEYWORD)};
    ONLY_CONTINUE_IF(oneof_value && oneof_value->is_array() &&
                     oneof_value->size() > 1);

    const auto has_validation_vocabulary{vocabularies.contains_any(
        {Vocabularies::Known::JSON_Schema_2020_12_Validation,
         Vocabularies::Known::JSON_Schema_2019_09_Validation,
         Vocabularies::Known::JSON_Schema_Draft_7,
         Vocabularies::Known::JSON_Schema_Draft_6,
         Vocabularies::Known::JSON_Schema_Draft_4,
         Vocabularies::Known::JSON_Schema_Draft_2,
         Vocabularies::Known::JSON_Schema_Draft_1})};

    const auto has_const_vocabulary{vocabularies.contains_any(
        {Vocabularies::Known::JSON_Schema_2020_12_Validation,
         Vocabularies::Known::JSON_Schema_2019_09_Validation,
         Vocabularies::Known::JSON_Schema_Draft_7,
         Vocabularies::Known::JSON_Schema_Draft_6})};

    std::vector<sourcemeta::core::JSON::TypeSet> type_sets;
    type_sets.reserve(oneof_value->size());

    for (const auto &branch : oneof_value->as_array()) {
      ONLY_CONTINUE_IF(branch.is_object());

      const auto *type_value{branch.try_at("type")};
      const auto *const_value{has_const_vocabulary ? branch.try_at("const")
                                                   : nullptr};
      const auto *enum_value{has_validation_vocabulary ? branch.try_at("enum")
                                                       : nullptr};
      const auto has_enum{enum_value && enum_value->is_array()};

      if (type_value) {
        const auto branch_types{parse_schema_type(*type_value)};
        ONLY_CONTINUE_IF(branch_types.any());
        type_sets.push_back(branch_types);
      } else if (const_value && !has_enum) {
        sourcemeta::core::JSON::TypeSet branch_types;
        branch_types.set(std::to_underlying(const_value->type()));
        type_sets.push_back(branch_types);
      } else if (has_enum && !const_value) {
        sourcemeta::core::JSON::TypeSet branch_types;
        for (const auto &item : enum_value->as_array()) {
          branch_types.set(std::to_underlying(item.type()));
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

    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.rename("oneOf", "anyOf");
  }

  [[nodiscard]] auto rereference(const std::string_view,
                                 const sourcemeta::core::Pointer &origin
                                 [[maybe_unused]],
                                 const sourcemeta::core::Pointer &target,
                                 const sourcemeta::core::Pointer &current) const
      -> std::optional<sourcemeta::core::Pointer> override {
    const sourcemeta::core::Pointer oneof_prefix{current.concat("oneOf")};
    const sourcemeta::core::Pointer anyof_prefix{current.concat("anyOf")};
    return target.rebase(oneof_prefix, anyof_prefix);
  }
};
