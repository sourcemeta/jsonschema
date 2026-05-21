class DependentRequiredTautology final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::false_type;
  DependentRequiredTautology()
      : SchemaTransformRule{
            "dependent_required_tautology",
            "Defining requirements for a property using `dependentRequired` "
            "that is already marked as required is an unnecessarily complex "
            "use of `dependentRequired`"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Validation,
             Vocabularies::Known::JSON_Schema_2019_09_Validation}) &&
        schema.is_object());

    const auto *dependent_required{schema.try_at("dependentRequired")};
    ONLY_CONTINUE_IF(dependent_required && dependent_required->is_object());
    const auto *required{schema.try_at("required")};
    ONLY_CONTINUE_IF(required && required->is_array());

    ONLY_CONTINUE_IF(
        std::any_of(required->as_array().cbegin(), required->as_array().cend(),
                    [dependent_required](const auto &element) {
                      return element.is_string() &&
                             dependent_required->defines(element.to_string());
                    }));
    return APPLIES_TO_KEYWORDS("dependentRequired", "required");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    auto requirements{schema.at("required")};
    while (true) {
      bool match{false};
      const auto copy{requirements};
      for (const auto &element : copy.as_array()) {
        if (!element.is_string() ||
            !schema.at("dependentRequired").defines(element.to_string())) {
          continue;
        }

        const auto &dependents{
            schema.at("dependentRequired").at(element.to_string())};
        if (dependents.is_array()) {
          for (const auto &dependent : dependents.as_array()) {
            if (dependent.is_string()) {
              match = true;
              requirements.push_back(dependent);
            }
          }

          schema.at("dependentRequired").erase(element.to_string());
        }
      }

      if (!match) {
        break;
      }
    }

    schema.assign("required", requirements);
  }
};
