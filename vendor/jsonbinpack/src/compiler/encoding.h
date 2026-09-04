#ifndef SOURCEMETA_JSONBINPACK_COMPILER_ENCODING_H_
#define SOURCEMETA_JSONBINPACK_COMPILER_ENCODING_H_

#include <sourcemeta/blaze/foundation.h>
#include <sourcemeta/core/json.h>

namespace sourcemeta::jsonbinpack {

constexpr auto ENCODING_V1{"tag:sourcemeta.com,2024:jsonbinpack/encoding/v1"};

inline auto make_resolver(const sourcemeta::blaze::SchemaResolver &fallback)
    -> auto {
  return [&fallback](std::string_view identifier)
             -> sourcemeta::blaze::SchemaResolverResult {
    if (identifier == ENCODING_V1) {
      static const auto SCHEMA{sourcemeta::core::parse_json(R"JSON({
        "$id": "tag:sourcemeta.com,2024:jsonbinpack/encoding/v1",
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "$vocabulary": {
          "https://json-schema.org/draft/2020-12/vocab/core": true,
          "tag:sourcemeta.com,2024:jsonbinpack/encoding/v1": true
        }
      })JSON")};
      return SCHEMA;
    }

    return fallback(identifier);
  };
}

} // namespace sourcemeta::jsonbinpack

#endif
