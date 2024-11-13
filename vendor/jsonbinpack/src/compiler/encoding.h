#ifndef SOURCEMETA_JSONBINPACK_COMPILER_ENCODING_H_
#define SOURCEMETA_JSONBINPACK_COMPILER_ENCODING_H_

#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

namespace sourcemeta::jsonbinpack {

constexpr auto ENCODING_V1{"tag:sourcemeta.com,2024:jsonbinpack/encoding/v1"};

inline auto
make_resolver(const sourcemeta::jsontoolkit::SchemaResolver &fallback) -> auto {
  return [&fallback](std::string_view identifier)
             -> std::optional<sourcemeta::jsontoolkit::JSON> {
    if (identifier == ENCODING_V1) {
      return sourcemeta::jsontoolkit::parse(R"JSON({
        "$id": "tag:sourcemeta.com,2024:jsonbinpack/encoding/v1",
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "$vocabulary": {
          "https://json-schema.org/draft/2020-12/vocab/core": true,
          "tag:sourcemeta.com,2024:jsonbinpack/encoding/v1": true
        }
      })JSON");
    } else {
      return fallback(identifier);
    }
  };
}

} // namespace sourcemeta::jsonbinpack

#endif
