class DependenciesPropertyTautology final : public SchemaTransformRule {
public:
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
    return contains_any(vocabularies,
                        {"http://json-schema.org/draft-07/schema#",
                         "http://json-schema.org/draft-06/schema#",
                         "http://json-schema.org/draft-04/schema#",
                         "http://json-schema.org/draft-03/schema#"}) &&
           schema.is_object() && schema.defines("dependencies") &&
           schema.at("dependencies").is_object() &&
           schema.defines("required") && schema.at("required").is_array() &&
           std::any_of(schema.at("required").as_array().cbegin(),
                       schema.at("required").as_array().cend(),
                       [&schema](const auto &element) {
                         return element.is_string() &&
                                schema.at("dependencies")
                                    .defines(element.to_string()) &&
                                (schema.at("dependencies")
                                     .at(element.to_string())
                                     .is_array() ||
                                 schema.at("dependencies")
                                     .at(element.to_string())
                                     .is_string());
                       });
  }

  auto transform(JSON &schema) const -> void override {
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
