class ValidDefault final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  ValidDefault(Compiler compiler = default_schema_compiler)
      : SchemaTransformRule{"valid_default", "Only set a `default` value that "
                                             "validates against the schema"},
        compiler_{std::move(compiler)} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &root,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &walker,
            const sourcemeta::core::SchemaResolver &resolver) const
      -> SchemaTransformRule::Result override {
    using Known = SchemaVocabularies::Known;
    // Technically, the `default` keyword goes back to Draft 1, but Blaze
    // only supports Draft 3 and later
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {Known::JSON_SCHEMA_2020_12_META_DATA,
             Known::JSON_SCHEMA_2019_09_META_DATA, Known::JSON_SCHEMA_DRAFT_7,
             Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_4,
             Known::JSON_SCHEMA_DRAFT_3, Known::JSON_SCHEMA_DRAFT_3_HYPER}) &&
        schema.is_object() && schema.defines("default"));

    if (vocabularies.contains_any(
            {Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_6,
             Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_3,
             Known::JSON_SCHEMA_DRAFT_3_HYPER})) {
      ONLY_CONTINUE_IF(!schema.defines("$ref"));
    }

    const auto &instance{schema.at("default")};

    if (frame.standalone()) {
      const auto base{frame.uri(location.pointer)};
      assert(base.has_value());
      Template schema_template;
      try {
        schema_template = compile(root, walker, resolver, this->compiler_,
                                  frame, base.value().get(), Mode::Exhaustive);
      } catch (const CompilerReferenceTargetNotSchemaError &) {
        throw;
      } catch (const sourcemeta::core::SchemaVocabularyError &) {
        throw;
      } catch (...) {
        return false;
      }
      SimpleOutput output{instance};
      Evaluator evaluator;
      const auto result{
          evaluator.validate(schema_template, instance, std::ref(output))};
      if (result) {
        return false;
      }

      std::ostringstream message;
      for (const auto &entry : output) {
        message << entry.message << "\n";
        message << "  at instance location \"";
        sourcemeta::core::stringify(entry.instance_location, message);
        message << "\"\n";
        message << "  at evaluate path \"";
        sourcemeta::core::stringify(entry.evaluate_path, message);
        message << "\"\n";
      }

      return {{{"default"}}, std::move(message).str()};
    }

    sourcemeta::core::WeakPointer base;
    const auto schema_template{compile_non_standalone_subschema(
        root, frame, location, walker, resolver, this->compiler_, base)};
    ONLY_CONTINUE_IF(schema_template.has_value());
    SimpleOutput output{instance, base};
    Evaluator evaluator;
    const auto result{evaluator.validate(schema_template.value(), instance,
                                         std::ref(output))};
    if (result) {
      return false;
    }

    std::ostringstream message;
    for (const auto &entry : output) {
      message << entry.message << "\n";
      message << "  at instance location \"";
      sourcemeta::core::stringify(entry.instance_location, message);
      message << "\"\n";
      message << "  at evaluate path \"";
      sourcemeta::core::stringify(entry.evaluate_path, message);
      message << "\"\n";
    }

    return {{{"default"}}, std::move(message).str()};
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("default");
  }

private:
  const Compiler compiler_;
};
