class ImplicitObjectKeywords final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  ImplicitObjectKeywords() : SchemaTransformRule{"implicit_object_keywords"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::SchemaVocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type && type->is_string());

    const auto &type_value{type->to_string()};
    this->reset();

    if (type_value == "object") {
      this->check_object(schema, vocabularies);
    } else if (type_value == "array") {
      this->check_array(schema, vocabularies);
    }

    ONLY_CONTINUE_IF(this->has_work_);
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    // Object keywords
    if (this->add_pattern_properties_) {
      schema.assign("patternProperties", sourcemeta::core::JSON::make_object());
    }

    if (this->add_property_names_) {
      schema.assign("propertyNames", sourcemeta::core::JSON{true});
    }

    if (this->add_min_properties_) {
      const auto *required{schema.try_at("required")};
      schema.assign(
          "minProperties",
          sourcemeta::core::JSON{required && is_property_name_array(*required)
                                     ? required->size()
                                     : 0});
    }

    if (this->add_properties_) {
      schema.assign("properties", sourcemeta::core::JSON::make_object());
    }

    if (this->add_additional_properties_) {
      schema.assign("additionalProperties",
                    this->additional_properties_as_object_
                        ? sourcemeta::core::JSON::make_object()
                        : sourcemeta::core::JSON{true});
    }

    // Array keywords
    if (this->add_unique_items_) {
      schema.assign("uniqueItems", sourcemeta::core::JSON{false});
    }

    if (this->add_items_) {
      schema.assign("items", this->items_as_object_
                                 ? sourcemeta::core::JSON::make_object()
                                 : sourcemeta::core::JSON{true});
    }

    if (this->add_min_items_) {
      schema.assign("minItems", sourcemeta::core::JSON{0});
    }
  }

private:
  auto reset() const -> void {
    this->has_work_ = false;
    this->add_pattern_properties_ = false;
    this->add_property_names_ = false;
    this->add_min_properties_ = false;
    this->add_properties_ = false;
    this->add_additional_properties_ = false;
    this->additional_properties_as_object_ = false;
    this->add_unique_items_ = false;
    this->add_items_ = false;
    this->items_as_object_ = false;
    this->add_min_items_ = false;
  }

  auto
  check_object(const sourcemeta::core::JSON &schema,
               const sourcemeta::blaze::SchemaVocabularies &vocabularies) const
      -> void {
    this->add_pattern_properties_ =
        !schema.defines("patternProperties") &&
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_Schema_Draft_3,
             SchemaVocabularies::Known::JSON_Schema_Draft_4,
             SchemaVocabularies::Known::JSON_Schema_Draft_6,
             SchemaVocabularies::Known::JSON_Schema_Draft_7,
             SchemaVocabularies::Known::JSON_Schema_2019_09_Applicator,
             SchemaVocabularies::Known::JSON_Schema_2020_12_Applicator});

    this->add_property_names_ =
        !schema.defines("propertyNames") &&
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_Schema_Draft_6,
             SchemaVocabularies::Known::JSON_Schema_Draft_7,
             SchemaVocabularies::Known::JSON_Schema_2019_09_Applicator,
             SchemaVocabularies::Known::JSON_Schema_2020_12_Applicator});

    this->add_min_properties_ =
        !schema.defines("minProperties") &&
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_Schema_2020_12_Validation,
             SchemaVocabularies::Known::JSON_Schema_2019_09_Validation,
             SchemaVocabularies::Known::JSON_Schema_Draft_7,
             SchemaVocabularies::Known::JSON_Schema_Draft_6,
             SchemaVocabularies::Known::JSON_Schema_Draft_4});

    this->add_properties_ =
        !schema.defines("properties") &&
        ((vocabularies.contains(
              SchemaVocabularies::Known::JSON_Schema_2020_12_Validation) &&
          vocabularies.contains(
              SchemaVocabularies::Known::JSON_Schema_2020_12_Applicator)) ||
         (vocabularies.contains(
              SchemaVocabularies::Known::JSON_Schema_2019_09_Validation) &&
          vocabularies.contains(
              SchemaVocabularies::Known::JSON_Schema_2019_09_Applicator)) ||
         vocabularies.contains_any(
             {SchemaVocabularies::Known::JSON_Schema_Draft_7,
              SchemaVocabularies::Known::JSON_Schema_Draft_6,
              SchemaVocabularies::Known::JSON_Schema_Draft_4,
              SchemaVocabularies::Known::JSON_Schema_Draft_3,
              SchemaVocabularies::Known::JSON_Schema_Draft_2,
              SchemaVocabularies::Known::JSON_Schema_Draft_1,
              SchemaVocabularies::Known::JSON_Schema_Draft_0}));

    const bool is_legacy{vocabularies.contains_any(
        {SchemaVocabularies::Known::JSON_Schema_Draft_0,
         SchemaVocabularies::Known::JSON_Schema_Draft_1,
         SchemaVocabularies::Known::JSON_Schema_Draft_2,
         SchemaVocabularies::Known::JSON_Schema_Draft_3,
         SchemaVocabularies::Known::JSON_Schema_Draft_4,
         SchemaVocabularies::Known::JSON_Schema_Draft_6,
         SchemaVocabularies::Known::JSON_Schema_Draft_7})};

    this->add_additional_properties_ =
        is_legacy && !schema.defines("additionalProperties");
    this->additional_properties_as_object_ = vocabularies.contains_any(
        {SchemaVocabularies::Known::JSON_Schema_Draft_0,
         SchemaVocabularies::Known::JSON_Schema_Draft_1,
         SchemaVocabularies::Known::JSON_Schema_Draft_2,
         SchemaVocabularies::Known::JSON_Schema_Draft_3});

    this->has_work_ = this->add_pattern_properties_ ||
                      this->add_property_names_ || this->add_min_properties_ ||
                      this->add_properties_ || this->add_additional_properties_;
  }

  auto
  check_array(const sourcemeta::core::JSON &schema,
              const sourcemeta::blaze::SchemaVocabularies &vocabularies) const
      -> void {
    if (!vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_Schema_Draft_0,
             SchemaVocabularies::Known::JSON_Schema_Draft_1,
             SchemaVocabularies::Known::JSON_Schema_Draft_2,
             SchemaVocabularies::Known::JSON_Schema_Draft_3,
             SchemaVocabularies::Known::JSON_Schema_Draft_4,
             SchemaVocabularies::Known::JSON_Schema_Draft_6,
             SchemaVocabularies::Known::JSON_Schema_Draft_7,
             SchemaVocabularies::Known::JSON_Schema_2019_09_Applicator,
             SchemaVocabularies::Known::JSON_Schema_2020_12_Applicator})) {
      return;
    }

    const bool is_modern{vocabularies.contains_any(
        {SchemaVocabularies::Known::JSON_Schema_2019_09_Applicator,
         SchemaVocabularies::Known::JSON_Schema_2020_12_Applicator})};
    // `items` takes a schema, and no dialect before Draft 6 has boolean
    // schemas to offer it. `additionalProperties` is different: every dialect
    // that has it accepts a boolean there
    const bool without_boolean_schemas{vocabularies.contains_any(
        {SchemaVocabularies::Known::JSON_Schema_Draft_0,
         SchemaVocabularies::Known::JSON_Schema_Draft_1,
         SchemaVocabularies::Known::JSON_Schema_Draft_2,
         SchemaVocabularies::Known::JSON_Schema_Draft_3,
         SchemaVocabularies::Known::JSON_Schema_Draft_4})};

    this->add_unique_items_ =
        !schema.defines("uniqueItems") &&
        !vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_Schema_Draft_0,
             SchemaVocabularies::Known::JSON_Schema_Draft_1});

    this->add_items_ = !is_modern && !schema.defines("items");
    this->items_as_object_ = without_boolean_schemas;

    this->add_min_items_ = !schema.defines("minItems");

    this->has_work_ =
        this->add_unique_items_ || this->add_items_ || this->add_min_items_;
  }

  mutable bool has_work_{false};
  // Object
  mutable bool add_pattern_properties_{false};
  mutable bool add_property_names_{false};
  mutable bool add_min_properties_{false};
  mutable bool add_properties_{false};
  mutable bool add_additional_properties_{false};
  mutable bool additional_properties_as_object_{false};
  // Array
  mutable bool add_unique_items_{false};
  mutable bool add_items_{false};
  mutable bool items_as_object_{false};
  mutable bool add_min_items_{false};
};
