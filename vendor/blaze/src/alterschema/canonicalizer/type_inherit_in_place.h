class TypeInheritInPlace final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  TypeInheritInPlace()
      : SchemaTransformRule{
            "type_inherit_in_place",
            "An untyped schema inside an in-place applicator inherits "
            "the type from its nearest typed ancestor"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &root,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &walker,
            const sourcemeta::core::SchemaResolver &resolver) const
      -> SchemaTransformRule::Result override {
    using namespace sourcemeta::core;
    ONLY_CONTINUE_IF(schema.is_object());
    ONLY_CONTINUE_IF(vocabularies.contains_any(
        {Vocabularies::Known::JSON_Schema_2020_12_Validation,
         Vocabularies::Known::JSON_Schema_2019_09_Validation,
         Vocabularies::Known::JSON_Schema_Draft_7,
         Vocabularies::Known::JSON_Schema_Draft_6,
         Vocabularies::Known::JSON_Schema_Draft_4,
         Vocabularies::Known::JSON_Schema_Draft_3,
         Vocabularies::Known::JSON_Schema_Draft_2,
         Vocabularies::Known::JSON_Schema_Draft_1,
         Vocabularies::Known::JSON_Schema_Draft_0}));
    ONLY_CONTINUE_IF(!schema.defines("type"));
    ONLY_CONTINUE_IF(!schema.defines("enum"));
    ONLY_CONTINUE_IF(!vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Validation,
                          Vocabularies::Known::JSON_Schema_2019_09_Validation,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6}) ||
                     !schema.defines("const"));

    for (const auto &entry : schema.as_object()) {
      const auto &keyword_type{walker(entry.first, vocabularies).type};
      ONLY_CONTINUE_IF(keyword_type != SchemaKeywordType::Reference);
      ONLY_CONTINUE_IF(keyword_type ==
                           SchemaKeywordType::ApplicatorValueInPlaceOther ||
                       !IS_IN_PLACE_APPLICATOR(keyword_type));
    }

    // Walk up through in-place applicators excluding `allOf`. In `allOf` the
    // parent's type already constrains all branches (a conjunction), and other
    // rules may want to lift type out of conjunctions
    const auto ancestor{WALK_UP(
        root, frame, location, walker, resolver,
        [](const SchemaKeywordType keyword_type) {
          return IS_IN_PLACE_APPLICATOR(keyword_type) &&
                 keyword_type != SchemaKeywordType::ApplicatorElementsInPlace;
        },
        [](const JSON &ancestor_schema, const Vocabularies &) {
          return ancestor_schema.defines("type");
        })};

    if (ancestor.has_value()) {
      this->inherited_type_ = get(root, ancestor.value().get()).at("type");
      return true;
    }

    auto walk_pointer{location.pointer};
    auto walk_parent{location.parent};
    while (walk_parent.has_value()) {
      const auto &wp{walk_parent.value()};
      const auto walk_relative{walk_pointer.resolve_from(wp)};
      if (walk_relative.empty() || !walk_relative.at(0).is_property()) {
        break;
      }
      const auto walk_entry{frame.traverse(frame.uri(wp).value().get())};
      if (!walk_entry.has_value()) {
        break;
      }
      const auto walk_vocabularies{
          frame.vocabularies(walk_entry.value().get(), resolver)};
      const auto walk_keyword_type{
          walker(walk_relative.at(0).to_property(), walk_vocabularies).type};

      if (!IS_IN_PLACE_APPLICATOR(walk_keyword_type)) {
        break;
      }

      if (walk_keyword_type == SchemaKeywordType::ApplicatorElementsInPlace &&
          walk_relative.size() >= 2 && walk_relative.at(1).is_index()) {
        const auto branch_index{walk_relative.at(1).to_index()};
        const auto &allof_parent{get(root, wp)};
        const auto &keyword_name{walk_relative.at(0).to_property()};
        if (allof_parent.is_object() && allof_parent.defines(keyword_name)) {
          const auto &branches{allof_parent.at(keyword_name)};
          if (branches.is_array()) {
            for (std::size_t index = 0; index < branches.size(); ++index) {
              if (index == branch_index) {
                continue;
              }
              const auto &sibling{branches.at(index)};
              if (sibling.is_object() && sibling.defines("type") &&
                  sibling.at("type").is_string()) {
                this->inherited_type_ = sibling.at("type");
                return true;
              }
              if (sibling.is_object() && sibling.defines("enum") &&
                  sibling.at("enum").is_array() &&
                  !sibling.at("enum").empty()) {
                const auto inferred{infer_type_from_enum(sibling.at("enum"))};
                if (!inferred.empty()) {
                  this->inherited_type_ = JSON{inferred};
                  return true;
                }
              }
              if (sibling.is_object() && sibling.defines("$ref") &&
                  sibling.at("$ref").is_string()) {
                const auto ref_target{
                    frame.traverse(sibling.at("$ref").to_string())};
                if (ref_target.has_value()) {
                  const auto &ref_schema{
                      get(root, ref_target.value().get().pointer)};
                  if (ref_schema.is_object() && ref_schema.defines("type") &&
                      ref_schema.at("type").is_string()) {
                    this->inherited_type_ = ref_schema.at("type");
                    return true;
                  }
                }
              }
            }
          }
        }
      }

      walk_pointer = wp;
      walk_parent = walk_entry.value().get().parent;
    }

    return false;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.assign("type", this->inherited_type_);
  }

private:
  static auto infer_type_from_enum(const sourcemeta::core::JSON &enum_array)
      -> sourcemeta::core::JSON::String {
    using Type = sourcemeta::core::JSON::Type;
    bool all_null{true};
    bool all_boolean{true};
    bool all_integer{true};
    bool all_number{true};
    bool all_string{true};
    bool all_array{true};
    bool all_object{true};

    for (const auto &value : enum_array.as_array()) {
      const auto value_type{value.type()};
      if (value_type != Type::Null) {
        all_null = false;
      }
      if (value_type != Type::Boolean) {
        all_boolean = false;
      }
      if (value_type != Type::Integer) {
        all_integer = false;
      }
      if (value_type != Type::Integer && value_type != Type::Real) {
        all_number = false;
      }
      if (value_type != Type::String) {
        all_string = false;
      }
      if (value_type != Type::Array) {
        all_array = false;
      }
      if (value_type != Type::Object) {
        all_object = false;
      }
    }

    if (all_string) {
      return "string";
    }
    if (all_integer) {
      return "integer";
    }
    if (all_number) {
      return "number";
    }
    if (all_object) {
      return "object";
    }
    if (all_array) {
      return "array";
    }
    if (all_null) {
      return "null";
    }
    if (all_boolean) {
      return "boolean";
    }
    return "";
  }

  mutable sourcemeta::core::JSON inherited_type_{
      sourcemeta::core::JSON{nullptr}};
};
