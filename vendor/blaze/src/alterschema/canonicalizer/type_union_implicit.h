class TypeUnionImplicit final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  TypeUnionImplicit()
      : SchemaTransformRule{
            "type_union_implicit",
            "Not setting `type` is equivalent to accepting any type"} {};

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
    ONLY_CONTINUE_IF(schema.is_object() && !schema.empty());
    ONLY_CONTINUE_IF(!vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_Draft_0,
                          Vocabularies::Known::JSON_Schema_Draft_1,
                          Vocabularies::Known::JSON_Schema_Draft_2,
                          Vocabularies::Known::JSON_Schema_Draft_3}) ||
                     !schema.defines("disallow"));
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
      ONLY_CONTINUE_IF(
          // Applicators like `contentSchema` applies to decoded content, not
          // the current instance
          keyword_type == SchemaKeywordType::ApplicatorValueInPlaceOther ||
          !IS_IN_PLACE_APPLICATOR(keyword_type));
    }

    ONLY_CONTINUE_IF(!this->allof_sibling_constrains_type(root, frame, location,
                                                          walker, resolver));

    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    auto types{sourcemeta::core::JSON::make_array()};

    types.push_back(sourcemeta::core::JSON{"null"});
    types.push_back(sourcemeta::core::JSON{"boolean"});
    types.push_back(sourcemeta::core::JSON{"object"});
    types.push_back(sourcemeta::core::JSON{"array"});
    types.push_back(sourcemeta::core::JSON{"string"});

    // Note we don't add `integer`, as its covered by `number`
    types.push_back(sourcemeta::core::JSON{"number"});

    schema.assign("type", std::move(types));
  }

private:
  static auto allof_sibling_constrains_type(
      const sourcemeta::core::JSON &root,
      const sourcemeta::core::SchemaFrame &frame,
      const sourcemeta::core::SchemaFrame::Location &location,
      const sourcemeta::core::SchemaWalker &walker,
      const sourcemeta::core::SchemaResolver &resolver) -> bool {
    using namespace sourcemeta::core;
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
        if (allof_parent.is_object() && allof_parent.defines(keyword_name) &&
            allof_parent.at(keyword_name).is_array()) {
          const auto &branches{allof_parent.at(keyword_name)};
          for (std::size_t index = 0; index < branches.size(); ++index) {
            if (index == branch_index) {
              continue;
            }
            const auto &sibling{branches.at(index)};
            if (!sibling.is_object()) {
              continue;
            }

            if (sibling.defines("type")) {
              return true;
            }

            if (sibling.defines("enum") && sibling.at("enum").is_array() &&
                !sibling.at("enum").empty()) {
              return true;
            }
          }
        }
      }

      walk_pointer = wp;
      walk_parent = walk_entry.value().get().parent;
    }
    return false;
  }
};
