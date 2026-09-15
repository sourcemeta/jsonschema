#include <sourcemeta/core/jsonschema.h>

#include <cstdint> // std::uint8_t

enum class KnownSchema : std::uint8_t {
  JSONSCHEMA_2020_12,
  HYPERSCHEMA_2020_12,
  JSONSCHEMA_2020_12_APPLICATOR,
  JSONSCHEMA_2020_12_CONTENT,
  JSONSCHEMA_2020_12_CORE,
  JSONSCHEMA_2020_12_FORMAT_ANNOTATION,
  JSONSCHEMA_2020_12_FORMAT_ASSERTION,
  JSONSCHEMA_2020_12_HYPER_SCHEMA,
  JSONSCHEMA_2020_12_META_DATA,
  JSONSCHEMA_2020_12_UNEVALUATED,
  JSONSCHEMA_2020_12_VALIDATION,
  LINKS_2020_12,
  JSONSCHEMA_2020_12_OUTPUT,

  JSONSCHEMA_2019_09,
  HYPERSCHEMA_2019_09,
  JSONSCHEMA_2019_09_APPLICATOR,
  JSONSCHEMA_2019_09_CONTENT,
  JSONSCHEMA_2019_09_CORE,
  JSONSCHEMA_2019_09_FORMAT,
  JSONSCHEMA_2019_09_HYPER_SCHEMA,
  JSONSCHEMA_2019_09_META_DATA,
  JSONSCHEMA_2019_09_VALIDATION,
  LINKS_2019_09,
  JSONSCHEMA_2019_09_OUTPUT,
  HYPERSCHEMA_2019_09_OUTPUT,

  JSONSCHEMA_DRAFT7,
  HYPERSCHEMA_DRAFT7,
  LINKS_DRAFT7,
  HYPERSCHEMA_DRAFT7_OUTPUT,

  JSONSCHEMA_DRAFT6,
  HYPERSCHEMA_DRAFT6,
  LINKS_DRAFT6,

  JSONSCHEMA_DRAFT4,
  HYPERSCHEMA_DRAFT4,
  LINKS_DRAFT4,

  JSONSCHEMA_DRAFT3,
  HYPERSCHEMA_DRAFT3,
  LINKS_DRAFT3,
  JSON_REF_DRAFT3,

  JSONSCHEMA_DRAFT2,
  HYPERSCHEMA_DRAFT2,
  LINKS_DRAFT2,
  JSON_REF_DRAFT2,

  JSONSCHEMA_DRAFT1,
  HYPERSCHEMA_DRAFT1,
  LINKS_DRAFT1,
  JSON_REF_DRAFT1,

  JSONSCHEMA_DRAFT0,
  HYPERSCHEMA_DRAFT0,
  LINKS_DRAFT0,
  JSON_REF_DRAFT0,

  OAS_3_2_DIALECT_2025_09_17,
  OAS_3_2_META_2025_09_17,

  OAS_3_1_DIALECT_BASE,
  OAS_3_1_META_BASE,

  UNKNOWN
};

static auto parse_identifier(const std::string_view identifier) -> KnownSchema {
  // JSON Schema 2020-12
  if (identifier == "https://json-schema.org/draft/2020-12/schema" ||
      identifier == "https://json-schema.org/draft/2020-12/schema#" ||
      identifier == "http://json-schema.org/draft/2020-12/schema" ||
      identifier == "http://json-schema.org/draft/2020-12/schema#") {
    return KnownSchema::JSONSCHEMA_2020_12;
  }

  if (identifier == "https://json-schema.org/draft/2020-12/hyper-schema" ||
      identifier == "https://json-schema.org/draft/2020-12/hyper-schema#" ||
      identifier == "http://json-schema.org/draft/2020-12/hyper-schema" ||
      identifier == "http://json-schema.org/draft/2020-12/hyper-schema#") {
    return KnownSchema::HYPERSCHEMA_2020_12;
  }

  if (identifier == "https://json-schema.org/draft/2020-12/meta/applicator" ||
      identifier == "http://json-schema.org/draft/2020-12/meta/applicator") {
    return KnownSchema::JSONSCHEMA_2020_12_APPLICATOR;
  }

  if (identifier == "https://json-schema.org/draft/2020-12/meta/content" ||
      identifier == "http://json-schema.org/draft/2020-12/meta/content") {
    return KnownSchema::JSONSCHEMA_2020_12_CONTENT;
  }

  if (identifier == "https://json-schema.org/draft/2020-12/meta/core" ||
      identifier == "http://json-schema.org/draft/2020-12/meta/core") {
    return KnownSchema::JSONSCHEMA_2020_12_CORE;
  }

  if (identifier ==
          "https://json-schema.org/draft/2020-12/meta/format-annotation" ||
      identifier ==
          "http://json-schema.org/draft/2020-12/meta/format-annotation") {
    return KnownSchema::JSONSCHEMA_2020_12_FORMAT_ANNOTATION;
  }

  if (identifier == "https://json-schema.org/draft/2020-12/meta/"
                    "format-assertion" ||
      identifier ==
          "http://json-schema.org/draft/2020-12/meta/format-assertion") {
    return KnownSchema::JSONSCHEMA_2020_12_FORMAT_ASSERTION;
  }

  if (identifier == "https://json-schema.org/draft/2020-12/meta/hyper-schema" ||
      identifier == "http://json-schema.org/draft/2020-12/meta/hyper-schema") {
    return KnownSchema::JSONSCHEMA_2020_12_HYPER_SCHEMA;
  }

  if (identifier == "https://json-schema.org/draft/2020-12/meta/meta-data" ||
      identifier == "http://json-schema.org/draft/2020-12/meta/meta-data") {
    return KnownSchema::JSONSCHEMA_2020_12_META_DATA;
  }

  if (identifier == "https://json-schema.org/draft/2020-12/meta/unevaluated" ||
      identifier == "http://json-schema.org/draft/2020-12/meta/unevaluated") {
    return KnownSchema::JSONSCHEMA_2020_12_UNEVALUATED;
  }

  if (identifier == "https://json-schema.org/draft/2020-12/meta/validation" ||
      identifier == "http://json-schema.org/draft/2020-12/meta/validation") {
    return KnownSchema::JSONSCHEMA_2020_12_VALIDATION;
  }

  if (identifier == "https://json-schema.org/draft/2020-12/links" ||
      identifier == "http://json-schema.org/draft/2020-12/links") {
    return KnownSchema::LINKS_2020_12;
  }

  if (identifier == "https://json-schema.org/draft/2020-12/output/schema" ||
      identifier == "http://json-schema.org/draft/2020-12/output/schema") {
    return KnownSchema::JSONSCHEMA_2020_12_OUTPUT;
  }

  // JSON Schema 2019-09
  if (identifier == "https://json-schema.org/draft/2019-09/schema" ||
      identifier == "https://json-schema.org/draft/2019-09/schema#" ||
      identifier == "http://json-schema.org/draft/2019-09/schema" ||
      identifier == "http://json-schema.org/draft/2019-09/schema#") {
    return KnownSchema::JSONSCHEMA_2019_09;
  }

  if (identifier == "https://json-schema.org/draft/2019-09/hyper-schema" ||
      identifier == "https://json-schema.org/draft/2019-09/hyper-schema#" ||
      identifier == "http://json-schema.org/draft/2019-09/hyper-schema" ||
      identifier == "http://json-schema.org/draft/2019-09/hyper-schema#") {
    return KnownSchema::HYPERSCHEMA_2019_09;
  }

  if (identifier == "https://json-schema.org/draft/2019-09/meta/applicator" ||
      identifier == "http://json-schema.org/draft/2019-09/meta/applicator") {
    return KnownSchema::JSONSCHEMA_2019_09_APPLICATOR;
  }

  if (identifier == "https://json-schema.org/draft/2019-09/meta/content" ||
      identifier == "http://json-schema.org/draft/2019-09/meta/content") {
    return KnownSchema::JSONSCHEMA_2019_09_CONTENT;
  }

  if (identifier == "https://json-schema.org/draft/2019-09/meta/core" ||
      identifier == "http://json-schema.org/draft/2019-09/meta/core") {
    return KnownSchema::JSONSCHEMA_2019_09_CORE;
  }

  if (identifier == "https://json-schema.org/draft/2019-09/meta/format" ||
      identifier == "http://json-schema.org/draft/2019-09/meta/format") {
    return KnownSchema::JSONSCHEMA_2019_09_FORMAT;
  }

  if (identifier == "https://json-schema.org/draft/2019-09/meta/hyper-schema" ||
      identifier == "http://json-schema.org/draft/2019-09/meta/hyper-schema") {
    return KnownSchema::JSONSCHEMA_2019_09_HYPER_SCHEMA;
  }

  if (identifier == "https://json-schema.org/draft/2019-09/meta/meta-data" ||
      identifier == "http://json-schema.org/draft/2019-09/meta/meta-data") {
    return KnownSchema::JSONSCHEMA_2019_09_META_DATA;
  }

  if (identifier == "https://json-schema.org/draft/2019-09/meta/validation" ||
      identifier == "http://json-schema.org/draft/2019-09/meta/validation") {
    return KnownSchema::JSONSCHEMA_2019_09_VALIDATION;
  }

  if (identifier == "https://json-schema.org/draft/2019-09/links" ||
      identifier == "http://json-schema.org/draft/2019-09/links") {
    return KnownSchema::LINKS_2019_09;
  }

  if (identifier == "https://json-schema.org/draft/2019-09/output/schema" ||
      identifier == "http://json-schema.org/draft/2019-09/output/schema") {
    return KnownSchema::JSONSCHEMA_2019_09_OUTPUT;
  }

  if (identifier ==
          "https://json-schema.org/draft/2019-09/output/hyper-schema" ||
      identifier ==
          "http://json-schema.org/draft/2019-09/output/hyper-schema") {
    return KnownSchema::HYPERSCHEMA_2019_09_OUTPUT;
  }

  // JSON Schema Draft7
  if (identifier == "http://json-schema.org/draft-07/schema#" ||
      identifier == "http://json-schema.org/draft-07/schema" ||
      identifier == "https://json-schema.org/draft-07/schema#" ||
      identifier == "https://json-schema.org/draft-07/schema") {
    return KnownSchema::JSONSCHEMA_DRAFT7;
  }

  if (identifier == "http://json-schema.org/draft-07/hyper-schema#" ||
      identifier == "http://json-schema.org/draft-07/hyper-schema" ||
      identifier == "https://json-schema.org/draft-07/hyper-schema#" ||
      identifier == "https://json-schema.org/draft-07/hyper-schema") {
    return KnownSchema::HYPERSCHEMA_DRAFT7;
  }

  if (identifier == "http://json-schema.org/draft-07/links#" ||
      identifier == "http://json-schema.org/draft-07/links" ||
      identifier == "https://json-schema.org/draft-07/links#" ||
      identifier == "https://json-schema.org/draft-07/links") {
    return KnownSchema::LINKS_DRAFT7;
  }

  if (identifier == "http://json-schema.org/draft-07/hyper-schema-output" ||
      identifier == "https://json-schema.org/draft-07/hyper-schema-output") {
    return KnownSchema::HYPERSCHEMA_DRAFT7_OUTPUT;
  }

  // JSON Schema Draft6
  if (identifier == "http://json-schema.org/draft-06/schema#" ||
      identifier == "http://json-schema.org/draft-06/schema" ||
      identifier == "https://json-schema.org/draft-06/schema#" ||
      identifier == "https://json-schema.org/draft-06/schema") {
    return KnownSchema::JSONSCHEMA_DRAFT6;
  }

  if (identifier == "http://json-schema.org/draft-06/hyper-schema#" ||
      identifier == "http://json-schema.org/draft-06/hyper-schema" ||
      identifier == "https://json-schema.org/draft-06/hyper-schema#" ||
      identifier == "https://json-schema.org/draft-06/hyper-schema") {
    return KnownSchema::HYPERSCHEMA_DRAFT6;
  }

  if (identifier == "http://json-schema.org/draft-06/links#" ||
      identifier == "http://json-schema.org/draft-06/links" ||
      identifier == "https://json-schema.org/draft-06/links#" ||
      identifier == "https://json-schema.org/draft-06/links") {
    return KnownSchema::LINKS_DRAFT6;
  }

  // JSON Schema Draft4
  if (identifier == "http://json-schema.org/draft-04/schema#" ||
      identifier == "http://json-schema.org/draft-04/schema" ||
      identifier == "https://json-schema.org/draft-04/schema#" ||
      identifier == "https://json-schema.org/draft-04/schema") {
    return KnownSchema::JSONSCHEMA_DRAFT4;
  }

  if (identifier == "http://json-schema.org/draft-04/hyper-schema#" ||
      identifier == "http://json-schema.org/draft-04/hyper-schema" ||
      identifier == "https://json-schema.org/draft-04/hyper-schema#" ||
      identifier == "https://json-schema.org/draft-04/hyper-schema") {
    return KnownSchema::HYPERSCHEMA_DRAFT4;
  }

  if (identifier == "http://json-schema.org/draft-04/links#" ||
      identifier == "http://json-schema.org/draft-04/links" ||
      identifier == "https://json-schema.org/draft-04/links#" ||
      identifier == "https://json-schema.org/draft-04/links") {
    return KnownSchema::LINKS_DRAFT4;
  }

  // JSON Schema Draft3
  if (identifier == "http://json-schema.org/draft-03/schema#" ||
      identifier == "http://json-schema.org/draft-03/schema" ||
      identifier == "https://json-schema.org/draft-03/schema#" ||
      identifier == "https://json-schema.org/draft-03/schema") {
    return KnownSchema::JSONSCHEMA_DRAFT3;
  }

  if (identifier == "http://json-schema.org/draft-03/hyper-schema#" ||
      identifier == "http://json-schema.org/draft-03/hyper-schema" ||
      identifier == "https://json-schema.org/draft-03/hyper-schema#" ||
      identifier == "https://json-schema.org/draft-03/hyper-schema") {
    return KnownSchema::HYPERSCHEMA_DRAFT3;
  }

  if (identifier == "http://json-schema.org/draft-03/links#" ||
      identifier == "http://json-schema.org/draft-03/links" ||
      identifier == "https://json-schema.org/draft-03/links#" ||
      identifier == "https://json-schema.org/draft-03/links") {
    return KnownSchema::LINKS_DRAFT3;
  }

  if (identifier == "http://json-schema.org/draft-03/json-ref#" ||
      identifier == "http://json-schema.org/draft-03/json-ref" ||
      identifier == "https://json-schema.org/draft-03/json-ref#" ||
      identifier == "https://json-schema.org/draft-03/json-ref") {
    return KnownSchema::JSON_REF_DRAFT3;
  }

  // JSON Schema Draft2
  if (identifier == "http://json-schema.org/draft-02/schema#" ||
      identifier == "http://json-schema.org/draft-02/schema" ||
      identifier == "https://json-schema.org/draft-02/schema#" ||
      identifier == "https://json-schema.org/draft-02/schema") {
    return KnownSchema::JSONSCHEMA_DRAFT2;
  }

  if (identifier == "http://json-schema.org/draft-02/hyper-schema#" ||
      identifier == "http://json-schema.org/draft-02/hyper-schema" ||
      identifier == "https://json-schema.org/draft-02/hyper-schema#" ||
      identifier == "https://json-schema.org/draft-02/hyper-schema") {
    return KnownSchema::HYPERSCHEMA_DRAFT2;
  }

  if (identifier == "http://json-schema.org/draft-02/links#" ||
      identifier == "http://json-schema.org/draft-02/links" ||
      identifier == "https://json-schema.org/draft-02/links#" ||
      identifier == "https://json-schema.org/draft-02/links") {
    return KnownSchema::LINKS_DRAFT2;
  }

  if (identifier == "http://json-schema.org/draft-02/json-ref#" ||
      identifier == "http://json-schema.org/draft-02/json-ref" ||
      identifier == "https://json-schema.org/draft-02/json-ref#" ||
      identifier == "https://json-schema.org/draft-02/json-ref") {
    return KnownSchema::JSON_REF_DRAFT2;
  }

  // JSON Schema Draft1
  if (identifier == "http://json-schema.org/draft-01/schema#" ||
      identifier == "http://json-schema.org/draft-01/schema" ||
      identifier == "https://json-schema.org/draft-01/schema#" ||
      identifier == "https://json-schema.org/draft-01/schema") {
    return KnownSchema::JSONSCHEMA_DRAFT1;
  }

  if (identifier == "http://json-schema.org/draft-01/hyper-schema#" ||
      identifier == "http://json-schema.org/draft-01/hyper-schema" ||
      identifier == "https://json-schema.org/draft-01/hyper-schema#" ||
      identifier == "https://json-schema.org/draft-01/hyper-schema") {
    return KnownSchema::HYPERSCHEMA_DRAFT1;
  }

  if (identifier == "http://json-schema.org/draft-01/links#" ||
      identifier == "http://json-schema.org/draft-01/links" ||
      identifier == "https://json-schema.org/draft-01/links#" ||
      identifier == "https://json-schema.org/draft-01/links") {
    return KnownSchema::LINKS_DRAFT1;
  }

  if (identifier == "http://json-schema.org/draft-01/json-ref#" ||
      identifier == "http://json-schema.org/draft-01/json-ref" ||
      identifier == "https://json-schema.org/draft-01/json-ref#" ||
      identifier == "https://json-schema.org/draft-01/json-ref") {
    return KnownSchema::JSON_REF_DRAFT1;
  }

  // JSON Schema Draft0
  if (identifier == "http://json-schema.org/draft-00/schema#" ||
      identifier == "http://json-schema.org/draft-00/schema" ||
      identifier == "https://json-schema.org/draft-00/schema#" ||
      identifier == "https://json-schema.org/draft-00/schema") {
    return KnownSchema::JSONSCHEMA_DRAFT0;
  }

  if (identifier == "http://json-schema.org/draft-00/hyper-schema#" ||
      identifier == "http://json-schema.org/draft-00/hyper-schema" ||
      identifier == "https://json-schema.org/draft-00/hyper-schema#" ||
      identifier == "https://json-schema.org/draft-00/hyper-schema") {
    return KnownSchema::HYPERSCHEMA_DRAFT0;
  }

  if (identifier == "http://json-schema.org/draft-00/links#" ||
      identifier == "http://json-schema.org/draft-00/links" ||
      identifier == "https://json-schema.org/draft-00/links#" ||
      identifier == "https://json-schema.org/draft-00/links") {
    return KnownSchema::LINKS_DRAFT0;
  }

  if (identifier == "http://json-schema.org/draft-00/json-ref#" ||
      identifier == "http://json-schema.org/draft-00/json-ref" ||
      identifier == "https://json-schema.org/draft-00/json-ref#" ||
      identifier == "https://json-schema.org/draft-00/json-ref") {
    return KnownSchema::JSON_REF_DRAFT0;
  }

  // OpenAPI v3.2
  if (identifier == "https://spec.openapis.org/oas/3.2/dialect/2025-09-17") {
    return KnownSchema::OAS_3_2_DIALECT_2025_09_17;
  }

  if (identifier == "https://spec.openapis.org/oas/3.2/meta/2025-09-17") {
    return KnownSchema::OAS_3_2_META_2025_09_17;
  }

  // OpenAPI v3.1
  if (identifier == "https://spec.openapis.org/oas/3.1/dialect/base") {
    return KnownSchema::OAS_3_1_DIALECT_BASE;
  }

  if (identifier == "https://spec.openapis.org/oas/3.1/meta/base") {
    return KnownSchema::OAS_3_1_META_BASE;
  }

  return KnownSchema::UNKNOWN;
}

auto sourcemeta::core::schema_resolver(const std::string_view identifier)
    -> sourcemeta::core::SchemaResolverResult {
  switch (parse_identifier(identifier)) {
    case KnownSchema::JSONSCHEMA_2020_12: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2020_12@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::HYPERSCHEMA_2020_12: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_HYPERSCHEMA_2020_12@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSONSCHEMA_2020_12_APPLICATOR: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_APPLICATOR@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSONSCHEMA_2020_12_CONTENT: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_CONTENT@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSONSCHEMA_2020_12_CORE: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_CORE@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSONSCHEMA_2020_12_FORMAT_ANNOTATION: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_FORMAT_ANNOTATION@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSONSCHEMA_2020_12_FORMAT_ASSERTION: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_FORMAT_ASSERTION@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSONSCHEMA_2020_12_HYPER_SCHEMA: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_HYPER_SCHEMA@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSONSCHEMA_2020_12_META_DATA: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_META_DATA@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSONSCHEMA_2020_12_UNEVALUATED: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_UNEVALUATED@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSONSCHEMA_2020_12_VALIDATION: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_VALIDATION@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::LINKS_2020_12: {
      static const auto SCHEMA{
          sourcemeta::core::parse_json(R"EOF(@METASCHEMA_LINKS_2020_12@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSONSCHEMA_2020_12_OUTPUT: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_OUTPUT@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSONSCHEMA_2019_09: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2019_09@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::HYPERSCHEMA_2019_09: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_HYPERSCHEMA_2019_09@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSONSCHEMA_2019_09_APPLICATOR: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2019_09_APPLICATOR@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSONSCHEMA_2019_09_CONTENT: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2019_09_CONTENT@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSONSCHEMA_2019_09_CORE: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2019_09_CORE@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSONSCHEMA_2019_09_FORMAT: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2019_09_FORMAT@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSONSCHEMA_2019_09_HYPER_SCHEMA: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2019_09_HYPER_SCHEMA@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSONSCHEMA_2019_09_META_DATA: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2019_09_META_DATA@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSONSCHEMA_2019_09_VALIDATION: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2019_09_VALIDATION@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::LINKS_2019_09: {
      static const auto SCHEMA{
          sourcemeta::core::parse_json(R"EOF(@METASCHEMA_LINKS_2019_09@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSONSCHEMA_2019_09_OUTPUT: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2019_09_OUTPUT@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::HYPERSCHEMA_2019_09_OUTPUT: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_HYPERSCHEMA_2019_09_OUTPUT@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSONSCHEMA_DRAFT7: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_DRAFT7@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::HYPERSCHEMA_DRAFT7: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_HYPERSCHEMA_DRAFT7@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::LINKS_DRAFT7: {
      static const auto SCHEMA{
          sourcemeta::core::parse_json(R"EOF(@METASCHEMA_LINKS_DRAFT7@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::HYPERSCHEMA_DRAFT7_OUTPUT: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_HYPERSCHEMA_DRAFT7_OUTPUT@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSONSCHEMA_DRAFT6: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_DRAFT6@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::HYPERSCHEMA_DRAFT6: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_HYPERSCHEMA_DRAFT6@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::LINKS_DRAFT6: {
      static const auto SCHEMA{
          sourcemeta::core::parse_json(R"EOF(@METASCHEMA_LINKS_DRAFT6@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSONSCHEMA_DRAFT4: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_DRAFT4@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::HYPERSCHEMA_DRAFT4: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_HYPERSCHEMA_DRAFT4@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::LINKS_DRAFT4: {
      static const auto SCHEMA{
          sourcemeta::core::parse_json(R"EOF(@METASCHEMA_LINKS_DRAFT4@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSONSCHEMA_DRAFT3: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_DRAFT3@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::HYPERSCHEMA_DRAFT3: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_HYPERSCHEMA_DRAFT3@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::LINKS_DRAFT3: {
      static const auto SCHEMA{
          sourcemeta::core::parse_json(R"EOF(@METASCHEMA_LINKS_DRAFT3@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSON_REF_DRAFT3: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSON_REF_DRAFT3@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSONSCHEMA_DRAFT2: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_DRAFT2@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::HYPERSCHEMA_DRAFT2: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_HYPERSCHEMA_DRAFT2@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::LINKS_DRAFT2: {
      static const auto SCHEMA{
          sourcemeta::core::parse_json(R"EOF(@METASCHEMA_LINKS_DRAFT2@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSON_REF_DRAFT2: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSON_REF_DRAFT2@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSONSCHEMA_DRAFT1: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_DRAFT1@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::HYPERSCHEMA_DRAFT1: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_HYPERSCHEMA_DRAFT1@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::LINKS_DRAFT1: {
      static const auto SCHEMA{
          sourcemeta::core::parse_json(R"EOF(@METASCHEMA_LINKS_DRAFT1@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSON_REF_DRAFT1: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSON_REF_DRAFT1@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSONSCHEMA_DRAFT0: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_DRAFT0@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::HYPERSCHEMA_DRAFT0: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_HYPERSCHEMA_DRAFT0@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::LINKS_DRAFT0: {
      static const auto SCHEMA{
          sourcemeta::core::parse_json(R"EOF(@METASCHEMA_LINKS_DRAFT0@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::JSON_REF_DRAFT0: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSON_REF_DRAFT0@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::OAS_3_2_DIALECT_2025_09_17: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@OPENAPI_OAS_3_2_DIALECT_2025_09_17@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::OAS_3_2_META_2025_09_17: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@OPENAPI_OAS_3_2_META_2025_09_17@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::OAS_3_1_DIALECT_BASE: {
      static const auto SCHEMA{sourcemeta::core::parse_json(
          R"EOF(@OPENAPI_OAS_3_1_DIALECT_BASE@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::OAS_3_1_META_BASE: {
      static const auto SCHEMA{
          sourcemeta::core::parse_json(R"EOF(@OPENAPI_OAS_3_1_META_BASE@)EOF")};
      return SCHEMA;
    }
    case KnownSchema::UNKNOWN:
      return std::nullopt;
  }

  return std::nullopt;
}

auto sourcemeta::core::schema_is_known(
    const std::string_view identifier) noexcept -> bool {
  return parse_identifier(identifier) != KnownSchema::UNKNOWN;
}

auto sourcemeta::core::schema_is_official(
    const std::string_view identifier) noexcept -> bool {
  switch (parse_identifier(identifier)) {
    case KnownSchema::JSONSCHEMA_2020_12:
    case KnownSchema::HYPERSCHEMA_2020_12:
    case KnownSchema::JSONSCHEMA_2020_12_APPLICATOR:
    case KnownSchema::JSONSCHEMA_2020_12_CONTENT:
    case KnownSchema::JSONSCHEMA_2020_12_CORE:
    case KnownSchema::JSONSCHEMA_2020_12_FORMAT_ANNOTATION:
    case KnownSchema::JSONSCHEMA_2020_12_FORMAT_ASSERTION:
    case KnownSchema::JSONSCHEMA_2020_12_HYPER_SCHEMA:
    case KnownSchema::JSONSCHEMA_2020_12_META_DATA:
    case KnownSchema::JSONSCHEMA_2020_12_UNEVALUATED:
    case KnownSchema::JSONSCHEMA_2020_12_VALIDATION:
    case KnownSchema::LINKS_2020_12:
    case KnownSchema::JSONSCHEMA_2020_12_OUTPUT:
    case KnownSchema::JSONSCHEMA_2019_09:
    case KnownSchema::HYPERSCHEMA_2019_09:
    case KnownSchema::JSONSCHEMA_2019_09_APPLICATOR:
    case KnownSchema::JSONSCHEMA_2019_09_CONTENT:
    case KnownSchema::JSONSCHEMA_2019_09_CORE:
    case KnownSchema::JSONSCHEMA_2019_09_FORMAT:
    case KnownSchema::JSONSCHEMA_2019_09_HYPER_SCHEMA:
    case KnownSchema::JSONSCHEMA_2019_09_META_DATA:
    case KnownSchema::JSONSCHEMA_2019_09_VALIDATION:
    case KnownSchema::LINKS_2019_09:
    case KnownSchema::JSONSCHEMA_2019_09_OUTPUT:
    case KnownSchema::HYPERSCHEMA_2019_09_OUTPUT:
    case KnownSchema::JSONSCHEMA_DRAFT7:
    case KnownSchema::HYPERSCHEMA_DRAFT7:
    case KnownSchema::LINKS_DRAFT7:
    case KnownSchema::HYPERSCHEMA_DRAFT7_OUTPUT:
    case KnownSchema::JSONSCHEMA_DRAFT6:
    case KnownSchema::HYPERSCHEMA_DRAFT6:
    case KnownSchema::LINKS_DRAFT6:
    case KnownSchema::JSONSCHEMA_DRAFT4:
    case KnownSchema::HYPERSCHEMA_DRAFT4:
    case KnownSchema::LINKS_DRAFT4:
    case KnownSchema::JSONSCHEMA_DRAFT3:
    case KnownSchema::HYPERSCHEMA_DRAFT3:
    case KnownSchema::LINKS_DRAFT3:
    case KnownSchema::JSON_REF_DRAFT3:
    case KnownSchema::JSONSCHEMA_DRAFT2:
    case KnownSchema::HYPERSCHEMA_DRAFT2:
    case KnownSchema::LINKS_DRAFT2:
    case KnownSchema::JSON_REF_DRAFT2:
    case KnownSchema::JSONSCHEMA_DRAFT1:
    case KnownSchema::HYPERSCHEMA_DRAFT1:
    case KnownSchema::LINKS_DRAFT1:
    case KnownSchema::JSON_REF_DRAFT1:
    case KnownSchema::JSONSCHEMA_DRAFT0:
    case KnownSchema::HYPERSCHEMA_DRAFT0:
    case KnownSchema::LINKS_DRAFT0:
    case KnownSchema::JSON_REF_DRAFT0:
      return true;
    case KnownSchema::OAS_3_2_DIALECT_2025_09_17:
    case KnownSchema::OAS_3_2_META_2025_09_17:
    case KnownSchema::OAS_3_1_DIALECT_BASE:
    case KnownSchema::OAS_3_1_META_BASE:
    case KnownSchema::UNKNOWN:
      return false;
  }

  return false;
}
