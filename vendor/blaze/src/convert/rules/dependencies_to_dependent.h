class DependenciesToDependent final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  DependenciesToDependent()
      : SchemaTransformRule{"dependencies_to_dependent"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_SCHEMA_2020_12_APPLICATOR,
             SchemaVocabularies::Known::JSON_SCHEMA_2019_09_APPLICATOR}) &&
        schema.is_object() && !schema.defines("dependentSchemas") &&
        !schema.defines("dependentRequired"));

    const auto *dependencies{schema.try_at("dependencies")};
    ONLY_CONTINUE_IF(dependencies && dependencies->is_object());

    for (const auto &entry : dependencies->as_object()) {
      if (!entry.second.is_array() && !entry.second.is_object() &&
          !entry.second.is_boolean()) {
        return false;
      }
    }

    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    this->renames_.clear();
    auto dependent_required{sourcemeta::core::JSON::make_object()};
    auto dependent_schemas{sourcemeta::core::JSON::make_object()};

    for (const auto &entry : schema.at("dependencies").as_object()) {
      if (entry.second.is_array()) {
        dependent_required.assign(entry.first, entry.second);
      } else {
        dependent_schemas.assign(entry.first, entry.second);
      }
    }

    // An empty container has nothing to sort into the two keywords that
    // replaced this one, and picking either would invent a claim the document
    // never made. A `definitions` has a single successor, which is why that
    // one is renamed rather than dropped
    if (dependent_required.empty() && dependent_schemas.empty()) {
      schema.erase("dependencies");
      return;
    }

    if (!dependent_required.empty() && !dependent_schemas.empty()) {
      for (const auto &entry : dependent_schemas.as_object()) {
        this->renames_.emplace_back(
            sourcemeta::core::Pointer{"dependencies", entry.first},
            sourcemeta::core::Pointer{"dependentSchemas", entry.first});
      }
      for (const auto &entry : dependent_required.as_object()) {
        this->renames_.emplace_back(
            sourcemeta::core::Pointer{"dependencies", entry.first},
            sourcemeta::core::Pointer{"dependentRequired", entry.first});
      }
      schema.try_assign_before("dependentSchemas", dependent_schemas,
                               "dependencies");
      schema.rename("dependencies", "dependentRequired");
      schema.at("dependentRequired").into(std::move(dependent_required));
      return;
    }

    if (!dependent_schemas.empty()) {
      this->renames_.emplace_back(
          sourcemeta::core::Pointer{"dependencies"},
          sourcemeta::core::Pointer{"dependentSchemas"});
      schema.rename("dependencies", "dependentSchemas");
      schema.at("dependentSchemas").into(std::move(dependent_schemas));
      return;
    }

    this->renames_.emplace_back(sourcemeta::core::Pointer{"dependencies"},
                                sourcemeta::core::Pointer{"dependentRequired"});
    schema.rename("dependencies", "dependentRequired");
    schema.at("dependentRequired").into(std::move(dependent_required));
  }

  [[nodiscard]] auto rereference(const std::string_view,
                                 const sourcemeta::core::Pointer &,
                                 const sourcemeta::core::Pointer &target,
                                 const sourcemeta::core::Pointer &current) const
      -> std::optional<sourcemeta::core::Pointer> override {
    for (const auto &[old_pointer, new_pointer] : this->renames_) {
      const auto result{target.rebase(current.concat(old_pointer),
                                      current.concat(new_pointer))};
      if (result != target) {
        return result;
      }
    }

    return target;
  }

private:
  mutable std::vector<
      std::pair<sourcemeta::core::Pointer, sourcemeta::core::Pointer>>
      renames_;
};
