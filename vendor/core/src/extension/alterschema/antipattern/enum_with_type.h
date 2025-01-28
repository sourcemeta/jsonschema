class EnumWithType final : public SchemaTransformRule {
public:
  EnumWithType()
      : SchemaTransformRule{
            "enum_with_type",
            "Setting `type` alongside `enum` is considered an anti-pattern, as "
            "the enumeration choices already imply their respective types"} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::core::Pointer &) const
      -> bool override {
    return contains_any(
               vocabularies,
               {"https://json-schema.org/draft/2020-12/vocab/validation",
                "https://json-schema.org/draft/2019-09/vocab/validation",
                "http://json-schema.org/draft-07/schema#",
                "http://json-schema.org/draft-06/schema#",
                "http://json-schema.org/draft-04/schema#",
                "http://json-schema.org/draft-03/schema#",
                "http://json-schema.org/draft-02/hyper-schema#",
                "http://json-schema.org/draft-01/hyper-schema#"}) &&
           schema.is_object() && schema.defines("type") &&
           schema.defines("enum") &&

           // Guard against cases where not all of the enumeration members
           // match the desired type, in which case the type is adding
           // an extra constraint and cannot be safely removed
           schema.at("type").is_string() && schema.at("enum").is_array() &&
           std::all_of(schema.at("enum").as_array().cbegin(),
                       schema.at("enum").as_array().cend(),
                       [&schema](const auto &item) {
                         const auto &type{schema.at("type").to_string()};
                         if (type == "object") {
                           return item.is_object();
                         } else if (type == "array") {
                           return item.is_array();
                         } else if (type == "string") {
                           return item.is_string();
                         } else if (type == "boolean") {
                           return item.is_boolean();
                         } else if (type == "null") {
                           return item.is_null();
                         } else if (type == "number") {
                           return item.is_number();
                         } else if (type == "integer") {
                           return item.is_integer();
                         } else {
                           return false;
                         }
                       });
  }

  auto transform(PointerProxy &transformer) const -> void override {
    transformer.erase("type");
  }
};
