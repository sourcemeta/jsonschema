class TypeUnionToSchemas final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  TypeUnionToSchemas() : SchemaTransformRule{"type_union_to_schemas"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any({Vocabularies::Known::JSON_Schema_Draft_0,
                                   Vocabularies::Known::JSON_Schema_Draft_1,
                                   Vocabularies::Known::JSON_Schema_Draft_2,
                                   Vocabularies::Known::JSON_Schema_Draft_3}) &&
        schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type && type->is_array());

    for (const auto &element : type->as_array()) {
      if (element.is_string()) {
        return true;
      }
    }

    return false;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    auto new_array{sourcemeta::core::JSON::make_array()};
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
  static auto type_string_to_schema(const std::string &type_name)
      -> sourcemeta::core::JSON {
    if (type_name == "null") {
      auto result{sourcemeta::core::JSON::make_object()};
      auto values{sourcemeta::core::JSON::make_array()};
      values.push_back(sourcemeta::core::JSON{nullptr});
      result.assign("enum", std::move(values));
      return result;
    }

    if (type_name == "boolean") {
      auto result{sourcemeta::core::JSON::make_object()};
      auto values{sourcemeta::core::JSON::make_array()};
      values.push_back(sourcemeta::core::JSON{false});
      values.push_back(sourcemeta::core::JSON{true});
      result.assign("enum", std::move(values));
      return result;
    }

    if (type_name == "any") {
      return sourcemeta::core::JSON::make_object();
    }

    auto result{sourcemeta::core::JSON::make_object()};
    result.assign("type", sourcemeta::core::JSON{type_name});
    return result;
  }
};
