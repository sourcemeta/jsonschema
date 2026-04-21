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
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &walker,
            const sourcemeta::core::SchemaResolver &resolver) const
      -> SchemaTransformRule::Result override {
    using Known = Vocabularies::Known;
    // Technically, the `default` keyword goes back to Draft 1, but Blaze
    // only supports Draft 4 and later
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {Known::JSON_Schema_2020_12_Meta_Data,
             Known::JSON_Schema_2019_09_Meta_Data, Known::JSON_Schema_Draft_7,
             Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_4}) &&
        schema.is_object() && schema.defines("default"));

    if (vocabularies.contains_any({Known::JSON_Schema_Draft_7,
                                   Known::JSON_Schema_Draft_6,
                                   Known::JSON_Schema_Draft_4})) {
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

    const auto &root_base_dialect{
        frame.traverse(frame.root()).value_or(location).get().base_dialect};
    std::string_view default_id{location.base};
    if (!sourcemeta::core::identify(root, root_base_dialect).empty() ||
        default_id.empty()) {
      default_id = "";
    }

    sourcemeta::core::WeakPointer base;
    const auto subschema{
        sourcemeta::core::wrap(root, frame, location, resolver, base)};
    Template schema_template;
    try {
      schema_template = compile(subschema, walker, resolver, this->compiler_,
                                Mode::Exhaustive, location.dialect, default_id);
    } catch (const CompilerReferenceTargetNotSchemaError &) {
      throw;
    } catch (const sourcemeta::core::SchemaVocabularyError &) {
      throw;
    } catch (...) {
      return false;
    }
    SimpleOutput output{instance, base};
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

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("default");
  }

private:
  const Compiler compiler_;
};
