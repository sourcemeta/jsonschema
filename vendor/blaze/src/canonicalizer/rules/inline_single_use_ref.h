class InlineSingleUseRef final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  InlineSingleUseRef() : SchemaTransformRule{"inline_single_use_ref"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &root,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &frame,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(schema.is_object() && schema.size() == 1);

    const auto *ref{schema.try_at("$ref")};
    ONLY_CONTINUE_IF(ref && ref->is_string());

    if (!location.parent.has_value()) {
      return false;
    }
    {
      const auto &parent_pointer{location.parent.value()};
      const auto relative{location.pointer.resolve_from(parent_pointer)};
      ONLY_CONTINUE_IF(!relative.empty() && relative.at(0).is_property() &&
                       relative.at(0).to_property() == "allOf" &&
                       relative.size() >= 2 && relative.at(1).is_index());
      const auto &parent_schema{sourcemeta::core::get(root, parent_pointer)};
      ONLY_CONTINUE_IF(parent_schema.is_object());
      const auto *parent_all_of{parent_schema.try_at("allOf")};
      ONLY_CONTINUE_IF(parent_all_of && parent_all_of->is_array());
      const auto current_index{relative.at(1).to_index()};
      bool has_typed_sibling{false};
      for (std::size_t index = 0; index < parent_all_of->size(); ++index) {
        if (index == current_index) {
          continue;
        }
        const auto &sibling{parent_all_of->at(index)};
        if (sibling.is_object() &&
            (sibling.defines("type") || sibling.defines("enum"))) {
          has_typed_sibling = true;
          break;
        }
      }
      ONLY_CONTINUE_IF(has_typed_sibling);
    }

    ONLY_CONTINUE_IF(vocabularies.contains_any(
        {Vocabularies::Known::JSON_Schema_2020_12_Core,
         Vocabularies::Known::JSON_Schema_2019_09_Core,
         Vocabularies::Known::JSON_Schema_Draft_7,
         Vocabularies::Known::JSON_Schema_Draft_6,
         Vocabularies::Known::JSON_Schema_Draft_4}));

    const auto target{frame.traverse(ref->to_string())};
    ONLY_CONTINUE_IF(target.has_value());
    const auto &target_pointer{target->get().pointer};

    if (target_pointer.size() < 2 || !target_pointer.at(0).is_property()) {
      return false;
    }
    const auto &container{target_pointer.at(0).to_property()};
    ONLY_CONTINUE_IF(container == "definitions" || container == "$defs");

    std::size_t ref_count{0};
    for (const auto &reference : frame.references()) {
      const auto dest{frame.traverse(reference.second.destination)};
      if (!dest.has_value()) {
        continue;
      }
      if (dest->get().pointer.starts_with(target_pointer) ||
          target_pointer.starts_with(dest->get().pointer)) {
        ++ref_count;
      }
    }

    ONLY_CONTINUE_IF(ref_count == 1);

    const auto &target_schema{sourcemeta::core::get(root, target_pointer)};
    ONLY_CONTINUE_IF(!target_schema.is_boolean());
    ONLY_CONTINUE_IF(target_schema.is_object() &&
                     !target_schema.defines("type") &&
                     !target_schema.defines("enum"));
    ONLY_CONTINUE_IF((!target_schema.defines("$id") &&
                      !target_schema.defines("id") &&
                      !target_schema.defines("$anchor") &&
                      !target_schema.defines("$dynamicAnchor") &&
                      !target_schema.defines("$recursiveAnchor")));

    this->target_pointer_ = sourcemeta::core::to_pointer(target_pointer);
    this->target_copy_ = target_schema;
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.into(std::move(this->target_copy_));
  }

  [[nodiscard]] auto rereference(const std::string_view,
                                 const sourcemeta::core::Pointer &,
                                 const sourcemeta::core::Pointer &target,
                                 const sourcemeta::core::Pointer &current) const
      -> std::optional<sourcemeta::core::Pointer> override {
    if (target.starts_with(this->target_pointer_)) {
      const auto relative{target.resolve_from(this->target_pointer_)};
      return current.concat(relative);
    }
    return target;
  }

private:
  mutable sourcemeta::core::Pointer target_pointer_;
  mutable sourcemeta::core::JSON target_copy_{sourcemeta::core::JSON{nullptr}};
};
