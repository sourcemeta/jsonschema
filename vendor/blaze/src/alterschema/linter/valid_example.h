class ValidExample final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  ValidExample(Compiler compiler = default_schema_compiler)
      : SchemaTransformRule{"valid_example", "Only set an `example` value that "
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
    ONLY_CONTINUE_IF(vocabularies.contains_any({Known::OPENAPI_3_0_BASE,
                                                Known::OPENAPI_3_1_BASE,
                                                Known::OPENAPI_3_2_BASE}) &&
                     schema.is_object() && schema.defines("example"));

    if (vocabularies.contains(Known::OPENAPI_3_0_BASE)) {
      ONLY_CONTINUE_IF(!schema.defines("$ref"));
    }

    const auto &instance{schema.at("example")};

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

      return {{{"example"}}, std::move(message).str()};
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

    return {{{"example"}}, std::move(message).str()};
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("example");
  }

private:
  const Compiler compiler_;
};
