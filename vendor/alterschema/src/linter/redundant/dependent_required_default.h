class DependentRequiredDefault final : public Rule {
public:
  DependentRequiredDefault()
      : Rule{"dependent_required_default",
             "Setting the `dependentRequired` keyword to an empty object "
             "does not add any further constraint"} {};

  [[nodiscard]] auto condition(const sourcemeta::jsontoolkit::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::jsontoolkit::Pointer &) const
      -> bool override {
    return contains_any(
               vocabularies,
               {"https://json-schema.org/draft/2020-12/vocab/validation",
                "https://json-schema.org/draft/2019-09/vocab/validation"}) &&
           schema.is_object() && schema.defines("dependentRequired") &&
           schema.at("dependentRequired").is_object() &&
           schema.at("dependentRequired").empty();
  }

  auto transform(Transformer &transformer) const -> void override {
    transformer.erase("dependentRequired");
  }
};
