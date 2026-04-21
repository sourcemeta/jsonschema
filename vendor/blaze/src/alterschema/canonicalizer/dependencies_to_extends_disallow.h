class DependenciesToExtendsDisallow final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  DependenciesToExtendsDisallow()
      : SchemaTransformRule{"dependencies_to_extends_disallow", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains(Vocabularies::Known::JSON_Schema_Draft_3) &&
        schema.is_object() && schema.defines("dependencies") &&
        schema.at("dependencies").is_object());

    ONLY_CONTINUE_IF(std::ranges::any_of(
        schema.at("dependencies").as_object(), [](const auto &entry) {
          return is_schema(entry.second) || entry.second.is_array() ||
                 entry.second.is_string();
        }));
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    auto result_branches{JSON::make_array()};
    std::vector<JSON::String> processed;

    for (const auto &entry : schema.at("dependencies").as_object()) {
      auto required_property{JSON::make_object()};
      required_property.assign("type", JSON{"object"});
      auto required_props{JSON::make_object()};
      auto required_prop_schema{JSON::make_object()};
      required_prop_schema.assign("required", JSON{true});
      required_props.assign(entry.first, std::move(required_prop_schema));
      required_property.assign("properties", std::move(required_props));
      required_property.assign("patternProperties", JSON::make_object());
      required_property.assign("additionalProperties", JSON::make_object());

      auto not_required{JSON::make_object()};
      auto disallow_array{JSON::make_array()};
      disallow_array.push_back(required_property);
      not_required.assign("disallow", std::move(disallow_array));

      if (is_schema(entry.second)) {
        auto extends_array{JSON::make_array()};
        auto required_copy{JSON::make_object()};
        required_copy.assign("type", JSON{"object"});
        auto req_props_copy{JSON::make_object()};
        auto req_prop_schema_copy{JSON::make_object()};
        req_prop_schema_copy.assign("required", JSON{true});
        req_props_copy.assign(entry.first, std::move(req_prop_schema_copy));
        required_copy.assign("properties", std::move(req_props_copy));
        required_copy.assign("patternProperties", JSON::make_object());
        required_copy.assign("additionalProperties", JSON::make_object());
        extends_array.push_back(std::move(required_copy));
        extends_array.push_back(entry.second);

        auto extends_branch{JSON::make_object()};
        extends_branch.assign("extends", std::move(extends_array));

        auto type_array{JSON::make_array()};
        type_array.push_back(std::move(not_required));
        type_array.push_back(std::move(extends_branch));

        auto wrapper{JSON::make_object()};
        wrapper.assign("type", std::move(type_array));
        result_branches.push_back(std::move(wrapper));
      } else if (entry.second.is_string() || entry.second.is_array()) {
        std::vector<std::string> dependent_props;
        if (entry.second.is_string()) {
          dependent_props.push_back(entry.second.to_string());
        } else {
          for (const auto &dependent : entry.second.as_array()) {
            if (dependent.is_string()) {
              dependent_props.push_back(dependent.to_string());
            }
          }
        }

        auto required_all{JSON::make_object()};
        required_all.assign("type", JSON{"object"});
        auto all_props{JSON::make_object()};
        auto trigger_schema{JSON::make_object()};
        trigger_schema.assign("required", JSON{true});
        all_props.assign(entry.first, std::move(trigger_schema));
        for (const auto &dependent_prop : dependent_props) {
          auto dep_schema{JSON::make_object()};
          dep_schema.assign("required", JSON{true});
          all_props.assign(dependent_prop, std::move(dep_schema));
        }
        required_all.assign("properties", std::move(all_props));
        required_all.assign("patternProperties", JSON::make_object());
        required_all.assign("additionalProperties", JSON::make_object());

        auto type_array{JSON::make_array()};
        type_array.push_back(std::move(not_required));
        type_array.push_back(std::move(required_all));

        auto wrapper{JSON::make_object()};
        wrapper.assign("type", std::move(type_array));
        result_branches.push_back(std::move(wrapper));
      } else {
        continue;
      }

      processed.emplace_back(entry.first);
    }

    for (const auto &key : processed) {
      schema.at("dependencies").erase(key);
    }

    if (schema.at("dependencies").empty()) {
      schema.erase("dependencies");
    }

    if (schema.defines("extends") && schema.at("extends").is_array()) {
      for (auto &item : result_branches.as_array()) {
        schema.at("extends").push_back(std::move(item));
      }
    } else {
      schema.assign("extends", std::move(result_branches));
    }
  }
};
