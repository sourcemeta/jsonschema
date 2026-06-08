class UnevaluatedPropertiesToAdditionalProperties final
    : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  UnevaluatedPropertiesToAdditionalProperties()
      : SchemaTransformRule{"unevaluated_properties_to_additional_properties",
                            ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &walker,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Unevaluated,
             Vocabularies::Known::JSON_Schema_2019_09_Applicator}) &&
        schema.is_object() && schema.defines("unevaluatedProperties"));

    for (const auto &entry : schema.as_object()) {
      if (entry.first == "unevaluatedProperties") {
        continue;
      }
      const auto &metadata{walker(entry.first, vocabularies)};
      const auto keyword_type{metadata.type};
      if (keyword_type != sourcemeta::blaze::SchemaKeywordType::Unknown &&
          keyword_type != sourcemeta::blaze::SchemaKeywordType::Assertion &&
          keyword_type != sourcemeta::blaze::SchemaKeywordType::Annotation &&
          keyword_type != sourcemeta::blaze::SchemaKeywordType::Comment &&
          keyword_type != sourcemeta::blaze::SchemaKeywordType::Other &&
          keyword_type !=
              sourcemeta::blaze::SchemaKeywordType::LocationMembers) {
        return false;
      }
    }

    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.rename("unevaluatedProperties", "additionalProperties");
  }

  [[nodiscard]] auto rereference(const std::string_view, const Pointer &,
                                 const Pointer &target,
                                 const Pointer &current) const
      -> Pointer override {
    return target.rebase(current.concat({"unevaluatedProperties"}),
                         current.concat({"additionalProperties"}));
  }
};
