class DependentRequiredTautology final : public SchemaTransformRule {
public:
  DependentRequiredTautology()
      : SchemaTransformRule{
            "dependent_required_tautology",
            "Defining requirements for a property using `dependentRequired` "
            "that is already marked as required is an unnecessarily complex "
            "use of `dependentRequired`"} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::core::Pointer &) const
      -> bool override {
    return contains_any(
               vocabularies,
               {"https://json-schema.org/draft/2020-12/vocab/validation",
                "https://json-schema.org/draft/2019-09/vocab/validation"}) &&
           schema.is_object() && schema.defines("dependentRequired") &&
           schema.at("dependentRequired").is_object() &&
           schema.defines("required") && schema.at("required").is_array() &&
           std::any_of(schema.at("required").as_array().cbegin(),
                       schema.at("required").as_array().cend(),
                       [&schema](const auto &element) {
                         return element.is_string() &&
                                schema.at("dependentRequired")
                                    .defines(element.to_string());
                       });
  }

  auto transform(PointerProxy &transformer) const -> void override {
    auto requirements{transformer.value().at("required")};
    while (true) {
      bool match{false};
      const auto copy{requirements};
      for (const auto &element : copy.as_array()) {
        if (!element.is_string() || !transformer.value()
                                         .at("dependentRequired")
                                         .defines(element.to_string())) {
          continue;
        }

        const auto &dependents{transformer.value()
                                   .at("dependentRequired")
                                   .at(element.to_string())};
        if (dependents.is_array()) {
          for (const auto &dependent : dependents.as_array()) {
            if (dependent.is_string()) {
              match = true;
              requirements.push_back(dependent);
            }
          }

          transformer.erase({"dependentRequired"}, element.to_string());
        }
      }

      if (!match) {
        break;
      }
    }

    transformer.assign("required", requirements);
  }
};
