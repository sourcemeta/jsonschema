class UnevaluatedItemsToItems final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  UnevaluatedItemsToItems()
      : SchemaTransformRule{"unevaluated_items_to_items", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &walker,
            const sourcemeta::core::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Unevaluated,
             Vocabularies::Known::JSON_Schema_2019_09_Applicator}) &&
        schema.is_object() && schema.defines("unevaluatedItems"));

    for (const auto &entry : schema.as_object()) {
      if (entry.first == "unevaluatedItems") {
        continue;
      }
      const auto &metadata{walker(entry.first, vocabularies)};
      const auto keyword_type{metadata.type};
      if (keyword_type != sourcemeta::core::SchemaKeywordType::Unknown &&
          keyword_type != sourcemeta::core::SchemaKeywordType::Assertion &&
          keyword_type != sourcemeta::core::SchemaKeywordType::Annotation &&
          keyword_type != sourcemeta::core::SchemaKeywordType::Comment &&
          keyword_type != sourcemeta::core::SchemaKeywordType::Other &&
          keyword_type !=
              sourcemeta::core::SchemaKeywordType::LocationMembers) {
        return false;
      }
    }

    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.rename("unevaluatedItems", "items");
  }

  [[nodiscard]] auto rereference(const std::string_view, const Pointer &,
                                 const Pointer &target,
                                 const Pointer &current) const
      -> Pointer override {
    return target.rebase(current.concat({"unevaluatedItems"}),
                         current.concat({"items"}));
  }
};
