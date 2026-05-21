class UnsatisfiableInPlaceApplicatorType final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  UnsatisfiableInPlaceApplicatorType()
      : SchemaTransformRule{
            "unsatisfiable_in_place_applicator_type",
            "An in-place applicator branch that defines a `type` with no "
            "overlap with the parent `type` can never be satisfied"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &walker,
            const sourcemeta::blaze::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(schema.is_object() && schema.defines("type"));
    ONLY_CONTINUE_IF(vocabularies.contains_any(
        {Vocabularies::Known::JSON_Schema_2020_12_Validation,
         Vocabularies::Known::JSON_Schema_2019_09_Validation,
         Vocabularies::Known::JSON_Schema_Draft_7,
         Vocabularies::Known::JSON_Schema_Draft_6,
         Vocabularies::Known::JSON_Schema_Draft_4,
         Vocabularies::Known::JSON_Schema_Draft_3,
         Vocabularies::Known::JSON_Schema_Draft_3_Hyper,
         Vocabularies::Known::JSON_Schema_Draft_2,
         Vocabularies::Known::JSON_Schema_Draft_1,
         Vocabularies::Known::JSON_Schema_Draft_0}));
    const auto parent_types{parse_schema_type(schema.at("type"))};
    ONLY_CONTINUE_IF(parent_types.any());

    std::vector<Pointer> locations;

    for (const auto &entry : schema.as_object()) {
      const auto &keyword{entry.first};
      const auto &keyword_type{walker(keyword, vocabularies).type};

      if (keyword_type == SchemaKeywordType::ApplicatorElementsInPlace ||
          keyword_type == SchemaKeywordType::ApplicatorElementsInPlaceSome) {
        if (!entry.second.is_array()) {
          continue;
        }

        const auto &branches{entry.second};
        for (std::size_t index = 0; index < branches.size(); ++index) {
          const auto &branch{branches.at(index)};
          if (!branch.is_object()) {
            continue;
          }
          const auto *branch_type{branch.try_at("type")};
          if (!branch_type) {
            continue;
          }

          const auto branch_types{parse_schema_type(*branch_type)};
          if (branch_types.any() && (parent_types & branch_types).none()) {
            locations.push_back(Pointer{keyword, index});
          }
        }
      } else if (keyword_type ==
                     SchemaKeywordType::ApplicatorValueInPlaceMaybe ||
                 keyword_type ==
                     SchemaKeywordType::ApplicatorValueInPlaceNegate) {
        if (!entry.second.is_object()) {
          continue;
        }
        const auto *branch_type{entry.second.try_at("type")};
        if (!branch_type) {
          continue;
        }

        const auto branch_types{parse_schema_type(*branch_type)};
        if (branch_types.any() && (parent_types & branch_types).none()) {
          locations.push_back(Pointer{keyword});
        }
      }
    }

    ONLY_CONTINUE_IF(!locations.empty());
    return APPLIES_TO_POINTERS(std::move(locations));
  }

  auto transform(JSON &schema, const Result &result) const -> void override {
    for (const auto &location : result.locations) {
      if (location.size() == 2) {
        const auto &keyword{location.at(0).to_property()};
        const auto index{location.at(1).to_index()};
        schema.at(keyword).at(index).into(JSON{false});
      } else {
        assert(location.size() == 1);
        const auto &keyword{location.at(0).to_property()};
        schema.at(keyword).into(JSON{false});
      }
    }
  }
};
