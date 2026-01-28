class DependenciesPropertyTautology final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::false_type;
  DependenciesPropertyTautology()
      : SchemaTransformRule{
            "dependencies_property_tautology",
            "Defining requirements for a property using `dependencies` "
            "that is already marked as required is an unnecessarily complex "
            "use of `dependencies`"} {};

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
        vocabularies.contains_any({Vocabularies::Known::JSON_Schema_Draft_7,
                                   Vocabularies::Known::JSON_Schema_Draft_6,
                                   Vocabularies::Known::JSON_Schema_Draft_4,
                                   Vocabularies::Known::JSON_Schema_Draft_3}) &&
        schema.is_object() && schema.defines("dependencies") &&
        schema.at("dependencies").is_object() && schema.defines("required") &&
        schema.at("required").is_array());
    ONLY_CONTINUE_IF(std::ranges::any_of(
        schema.at("required").as_array(), [&schema](const auto &element) {
          return element.is_string() &&
                 schema.at("dependencies").defines(element.to_string()) &&
                 (schema.at("dependencies")
                      .at(element.to_string())
                      .is_array() ||
                  schema.at("dependencies")
                      .at(element.to_string())
                      .is_string());
        }));
    return APPLIES_TO_KEYWORDS("dependencies", "required");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    auto requirements{schema.at("required")};
    while (true) {
      bool match{false};
      const auto copy{requirements};
      for (const auto &element : copy.as_array()) {
        if (!element.is_string() ||
            !schema.at("dependencies").defines(element.to_string())) {
          continue;
        }

        const auto &dependents{
            schema.at("dependencies").at(element.to_string())};
        if (dependents.is_array()) {
          for (const auto &dependent : dependents.as_array()) {
            if (dependent.is_string()) {
              match = true;
              requirements.push_back(dependent);
            }
          }

          schema.at("dependencies").erase(element.to_string());
        } else if (dependents.is_string()) {
          match = true;
          requirements.push_back(dependents);
          schema.at("dependencies").erase(element.to_string());
        }
      }

      if (!match) {
        break;
      }
    }

    schema.assign("required", requirements);
  }
};
