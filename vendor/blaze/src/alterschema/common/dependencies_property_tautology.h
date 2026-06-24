class DependenciesPropertyTautology final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  DependenciesPropertyTautology()
      : SchemaTransformRule{
            "dependencies_property_tautology",
            "Defining requirements for a property using `dependencies` "
            "that is already marked as required is an unnecessarily complex "
            "use of `dependencies`"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4,
                          Vocabularies::Known::JSON_Schema_Draft_3,
                          Vocabularies::Known::JSON_Schema_Draft_3_Hyper}) &&
                     schema.is_object());

    const auto *dependencies{schema.try_at("dependencies")};
    ONLY_CONTINUE_IF(dependencies && dependencies->is_object());

    const bool is_draft_3{vocabularies.contains_any(
        {Vocabularies::Known::JSON_Schema_Draft_3,
         Vocabularies::Known::JSON_Schema_Draft_3_Hyper})};

    if (is_draft_3) {
      const auto *properties{schema.try_at("properties")};
      ONLY_CONTINUE_IF(properties && properties->is_object());

      ONLY_CONTINUE_IF(std::ranges::any_of(
          properties->as_object(), [dependencies](const auto &entry) -> auto {
            if (!entry.second.is_object()) {
              return false;
            }
            const auto *required{entry.second.try_at("required")};
            if (!required || !required->is_boolean() ||
                !required->to_boolean()) {
              return false;
            }
            const auto *dependent{dependencies->try_at(entry.first)};
            return dependent &&
                   (dependent->is_array() || dependent->is_string());
          }));
      return APPLIES_TO_KEYWORDS("dependencies", "properties");
    }

    const auto *required{schema.try_at("required")};
    ONLY_CONTINUE_IF(required && required->is_array());

    ONLY_CONTINUE_IF(std::ranges::any_of(
        required->as_array(), [dependencies](const auto &element) -> auto {
          if (!element.is_string()) {
            return false;
          }
          const auto *dependent{dependencies->try_at(element.to_string())};
          return dependent && (dependent->is_array() || dependent->is_string());
        }));
    return APPLIES_TO_KEYWORDS("dependencies", "required");
  }

  auto transform(JSON &schema, const Result &result) const -> void override {
    const bool is_draft_3_path{
        std::ranges::any_of(result.locations, [](const auto &pointer) -> auto {
          return pointer.size() == 1 && pointer.at(0).is_property() &&
                 pointer.at(0).to_property() == "properties";
        })};
    if (is_draft_3_path) {
      this->transform_boolean(schema);
    } else {
      this->transform_array(schema);
    }
  }

private:
  static auto transform_array(JSON &schema) -> void {
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

  static auto transform_boolean(JSON &schema) -> void {
    while (true) {
      bool match{false};

      std::vector<JSON::String> snapshot;
      for (const auto &entry : schema.at("properties").as_object()) {
        if (!entry.second.is_object()) {
          continue;
        }
        const auto *required{entry.second.try_at("required")};
        if (required && required->is_boolean() && required->to_boolean()) {
          snapshot.push_back(entry.first);
        }
      }

      for (const auto &name : snapshot) {
        if (!schema.at("dependencies").defines(name)) {
          continue;
        }

        const auto dependents_copy{schema.at("dependencies").at(name)};
        std::vector<JSON::String> new_required;
        if (dependents_copy.is_string()) {
          new_required.push_back(dependents_copy.to_string());
        } else if (dependents_copy.is_array()) {
          for (const auto &dependent : dependents_copy.as_array()) {
            if (dependent.is_string()) {
              new_required.push_back(dependent.to_string());
            }
          }
        } else {
          continue;
        }

        for (const auto &dependency_name : new_required) {
          if (!schema.at("properties").defines(dependency_name)) {
            auto new_property{JSON::make_object()};
            new_property.assign("required", JSON{true});
            schema.at("properties")
                .assign(dependency_name, std::move(new_property));
            match = true;
          } else if (schema.at("properties").at(dependency_name).is_object()) {
            auto &existing{schema.at("properties").at(dependency_name)};
            const auto *current_required{existing.try_at("required")};
            if (!current_required || !current_required->is_boolean() ||
                !current_required->to_boolean()) {
              existing.assign("required", JSON{true});
              match = true;
            }
          }
        }

        schema.at("dependencies").erase(name);
      }

      if (!match) {
        break;
      }
    }
  }
};
