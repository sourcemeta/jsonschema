class Draft3TypeAny final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  Draft3TypeAny() : SchemaTransformRule{"draft3_type_any", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains(Vocabularies::Known::JSON_Schema_Draft_3) &&
        schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type);

    if (type->is_string()) {
      return type->to_string() == "any";
    }

    if (type->is_array()) {
      for (const auto &element : type->as_array()) {
        if (element.is_string() && element.to_string() == "any") {
          return true;
        }
        if (element.is_object()) {
          if (element.empty()) {
            return true;
          }
          if (element.size() == 1) {
            const auto *element_type{element.try_at("type")};
            if (element_type && element_type->is_string() &&
                element_type->to_string() == "any") {
              return true;
            }
          }
        }
      }
    }

    return false;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("type");
  }
};
