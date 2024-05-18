#ifndef INTELLIGENCE_JSONSCHEMA_CLI_LINT_ENUM_WITH_TYPE_H_
#define INTELLIGENCE_JSONSCHEMA_CLI_LINT_ENUM_WITH_TYPE_H_

#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

namespace intelligence::jsonschema::cli {

class EnumWithType final : public sourcemeta::jsontoolkit::SchemaTransformRule {
public:
  EnumWithType()
      : SchemaTransformRule{"enum_with_type",
                            "Enumeration declarations imply their own types"} {
        };

  [[nodiscard]] auto
  condition(const sourcemeta::jsontoolkit::JSON &schema, const std::string &,
            const std::set<std::string> &vocabularies,
            const sourcemeta::jsontoolkit::Pointer &) const -> bool override {
    // TODO: This applies to older dialects too?
    return vocabularies.contains(
               "https://json-schema.org/draft/2020-12/vocab/validation") &&
           schema.is_object() && schema.defines("type") &&
           schema.defines("enum");
  }

  auto transform(sourcemeta::jsontoolkit::SchemaTransformer &transformer) const
      -> void override {
    transformer.erase("type");
  }
};

} // namespace intelligence::jsonschema::cli

#endif
