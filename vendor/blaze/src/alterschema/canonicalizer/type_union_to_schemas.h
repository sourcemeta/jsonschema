class TypeUnionToSchemas final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  TypeUnionToSchemas() : SchemaTransformRule{"type_union_to_schemas", ""} {};

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
        vocabularies.contains_any({Vocabularies::Known::JSON_Schema_Draft_0,
                                   Vocabularies::Known::JSON_Schema_Draft_1,
                                   Vocabularies::Known::JSON_Schema_Draft_2,
                                   Vocabularies::Known::JSON_Schema_Draft_3}) &&
        schema.is_object() && schema.defines("type") &&
        schema.at("type").is_array());

    for (const auto &element : schema.at("type").as_array()) {
      if (element.is_string()) {
        return true;
      }
    }

    return false;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    auto new_array{JSON::make_array()};
    for (const auto &element : schema.at("type").as_array()) {
      if (element.is_string()) {
        new_array.push_back(type_string_to_schema(element.to_string()));
      } else {
        new_array.push_back(element);
      }
    }
    schema.assign("type", std::move(new_array));
  }

private:
  static auto type_string_to_schema(const std::string &type_name) -> JSON {
    if (type_name == "null") {
      auto result{JSON::make_object()};
      auto values{JSON::make_array()};
      values.push_back(JSON{nullptr});
      result.assign("enum", std::move(values));
      return result;
    }

    if (type_name == "boolean") {
      auto result{JSON::make_object()};
      auto values{JSON::make_array()};
      values.push_back(JSON{false});
      values.push_back(JSON{true});
      result.assign("enum", std::move(values));
      return result;
    }

    if (type_name == "any") {
      return JSON::make_object();
    }

    auto result{JSON::make_object()};
    result.assign("type", JSON{type_name});
    return result;
  }
};
