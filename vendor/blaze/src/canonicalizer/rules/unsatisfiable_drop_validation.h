class UnsatisfiableDropValidation final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  UnsatisfiableDropValidation()
      : SchemaTransformRule{"unsatisfiable_drop_validation"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &frame,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &walker,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
                          Vocabularies::Known::JSON_Schema_2019_09_Applicator,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_3,
                          Vocabularies::Known::JSON_Schema_Draft_3_Hyper}) &&
                     schema.is_object());

    const bool is_draft_3{vocabularies.contains_any(
        {Vocabularies::Known::JSON_Schema_Draft_3,
         Vocabularies::Known::JSON_Schema_Draft_3_Hyper})};

    std::string_view trigger_keyword;
    if (is_draft_3) {
      const auto *disallow_value{schema.try_at("disallow")};
      if (disallow_value && is_disallow_tautology(*disallow_value)) {
        trigger_keyword = "disallow";
      }
    } else {
      const auto *not_value{schema.try_at("not")};
      if (not_value && sourcemeta::blaze::is_empty_schema(*not_value)) {
        trigger_keyword = "not";
      }
    }

    ONLY_CONTINUE_IF(!trigger_keyword.empty());

    std::vector<sourcemeta::core::Pointer> positions;
    for (const auto &entry : schema.as_object()) {
      if (entry.first == trigger_keyword) {
        continue;
      }

      const auto &metadata{walker(entry.first, vocabularies)};
      if (!is_removable_keyword_type(metadata.type)) {
        continue;
      }

      if (frame.has_references_through(
              location.pointer,
              sourcemeta::core::WeakPointer::Token{std::cref(entry.first)})) {
        continue;
      }

      positions.push_back(sourcemeta::core::Pointer{entry.first});
    }

    ONLY_CONTINUE_IF(!positions.empty());
    this->locations_ = std::move(positions);
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    for (const auto &location : this->locations_) {
      schema.erase(location.at(0).to_property());
    }
  }

private:
  static auto is_disallow_tautology(const sourcemeta::core::JSON &value)
      -> bool {
    if (value.is_string()) {
      return value.to_string() == "any";
    }
    if (sourcemeta::blaze::is_empty_schema(value)) {
      return true;
    }
    if (value.is_array()) {
      return std::ranges::any_of(
          value.as_array(), [](const auto &entry) -> auto {
            if (entry.is_string()) {
              return entry.to_string() == "any";
            }
            return sourcemeta::blaze::is_empty_schema(entry);
          });
    }
    return false;
  }

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

private:
  mutable std::vector<sourcemeta::core::Pointer> locations_;
};
