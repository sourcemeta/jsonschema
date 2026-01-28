class UnsatisfiableDropValidation final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  UnsatisfiableDropValidation()
      : SchemaTransformRule{"unsatisfiable_drop_validation",
                            "Do not place assertions or applicators next to an "
                            "unsatisfiable negation"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &walker,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
                          Vocabularies::Known::JSON_Schema_2019_09_Applicator,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6}) &&
                     schema.is_object() && schema.defines("not") &&
                     schema.at("not").is_boolean() &&
                     schema.at("not").to_boolean());

    std::vector<Pointer> positions;
    for (const auto &entry : schema.as_object()) {
      if (entry.first == "not") {
        continue;
      }

      const auto &metadata{walker(entry.first, vocabularies)};
      if (!is_removable_keyword_type(metadata.type)) {
        continue;
      }

      if (frame.has_references_through(
              location.pointer, WeakPointer::Token{std::cref(entry.first)})) {
        continue;
      }

      positions.push_back(Pointer{entry.first});
    }

    ONLY_CONTINUE_IF(!positions.empty());
    return APPLIES_TO_POINTERS(std::move(positions));
  }

  auto transform(JSON &schema, const Result &result) const -> void override {
    for (const auto &location : result.locations) {
      schema.erase(location.at(0).to_property());
    }
  }

private:
  static auto is_removable_keyword_type(const SchemaKeywordType type) -> bool {
    switch (type) {
      case SchemaKeywordType::Assertion:
      case SchemaKeywordType::Reference:
      case SchemaKeywordType::LocationMembers:
      case SchemaKeywordType::ApplicatorMembersTraversePropertyStatic:
      case SchemaKeywordType::ApplicatorMembersTraversePropertyRegex:
      case SchemaKeywordType::ApplicatorValueTraverseSomeProperty:
      case SchemaKeywordType::ApplicatorValueTraverseAnyPropertyKey:
      case SchemaKeywordType::ApplicatorValueTraverseAnyItem:
      case SchemaKeywordType::ApplicatorValueTraverseSomeItem:
      case SchemaKeywordType::ApplicatorValueTraverseParent:
      case SchemaKeywordType::ApplicatorElementsTraverseItem:
      case SchemaKeywordType::ApplicatorValueOrElementsTraverseAnyItemOrItem:
      case SchemaKeywordType::ApplicatorValueOrElementsInPlace:
      case SchemaKeywordType::ApplicatorMembersInPlaceSome:
      case SchemaKeywordType::ApplicatorElementsInPlace:
      case SchemaKeywordType::ApplicatorElementsInPlaceSome:
      case SchemaKeywordType::ApplicatorElementsInPlaceSomeNegate:
      case SchemaKeywordType::ApplicatorValueInPlaceMaybe:
      case SchemaKeywordType::ApplicatorValueInPlaceOther:
      case SchemaKeywordType::ApplicatorValueInPlaceNegate:
        return true;
      default:
        return false;
    }
  }
};
