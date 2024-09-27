#ifndef SOURCEMETA_JSONBINPACK_COMPILER_ENCODING_H_
#define SOURCEMETA_JSONBINPACK_COMPILER_ENCODING_H_

#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <future> // std::future

namespace sourcemeta::jsonbinpack {

constexpr auto ENCODING_V1{"tag:sourcemeta.com,2024:jsonbinpack/encoding/v1"};

inline auto
make_resolver(const sourcemeta::jsontoolkit::SchemaResolver &fallback) -> auto {
  return [&fallback](std::string_view identifier)
             -> std::future<std::optional<sourcemeta::jsontoolkit::JSON>> {
    std::promise<std::optional<sourcemeta::jsontoolkit::JSON>> promise;
    if (identifier == ENCODING_V1) {
      promise.set_value(sourcemeta::jsontoolkit::parse(R"JSON({
        "$id": "tag:sourcemeta.com,2024:jsonbinpack/encoding/v1",
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "$vocabulary": {
          "https://json-schema.org/draft/2020-12/vocab/core": true,
          "tag:sourcemeta.com,2024:jsonbinpack/encoding/v1": true
        }
      })JSON"));
    } else {
      promise.set_value(fallback(identifier).get());
    }

    return promise.get_future();
  };
}

} // namespace sourcemeta::jsonbinpack

#endif
