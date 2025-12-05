class DependentRequiredTautology final : public SchemaTransformRule {
public:
  DependentRequiredTautology()
      : SchemaTransformRule{
            "dependent_required_tautology",
            "Defining requirements for a property using `dependentRequired` "
            "that is already marked as required is an unnecessarily complex "
            "use of `dependentRequired`"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Validation,
             Vocabularies::Known::JSON_Schema_2019_09_Validation}) &&
        schema.is_object() && schema.defines("dependentRequired") &&
        schema.at("dependentRequired").is_object() &&
        schema.defines("required") && schema.at("required").is_array());
    ONLY_CONTINUE_IF(std::any_of(
        schema.at("required").as_array().cbegin(),
        schema.at("required").as_array().cend(),
        [&schema](const auto &element) {
          return element.is_string() &&
                 schema.at("dependentRequired").defines(element.to_string());
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
