#ifndef SOURCEMETA_BLAZE_BINPACK_H_
#define SOURCEMETA_BLAZE_BINPACK_H_

#ifndef SOURCEMETA_BLAZE_BINPACK_EXPORT
#include <sourcemeta/blaze/binpack_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/blaze/binpack_decoder.h>
#include <sourcemeta/blaze/binpack_encoder.h>
#include <sourcemeta/blaze/binpack_encoder_cache.h>
#include <sourcemeta/blaze/binpack_encoding.h>
#include <sourcemeta/blaze/binpack_error.h>
#include <sourcemeta/blaze/binpack_input_stream.h>
#include <sourcemeta/blaze/binpack_output_stream.h>
// NOLINTEND(misc-include-cleaner)

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <string_view> // std::string_view

/// @defgroup binpack BinPack
/// @brief A space-efficient binary JSON serialization format based on JSON
/// Schema
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/blaze/binpack.h>
/// ```
///
/// JSON BinPack is a binary JSON serialization format with a strong focus on
/// space efficiency. It encodes any JSON document, and it runs in both
/// schema-driven and schema-less modes given a matching JSON Schema 2020-12
/// definition.
///
/// ### Highly space-efficient
///
/// A reproducible benchmark study has proven JSON BinPack to be space-efficient
/// in comparison to 12 alternative binary serialization formats in every
/// considered case. It also typically provides a higher average size reduction
/// than general purpose compressors such as GZIP, LZMA and LZ4 configured with
/// their highest compression levels, specially for small and medium-sized
/// documents. For large documents, combining JSON BinPack with a compression
/// format may achieve greater space-efficiency still. The benchmark is live at
/// https://benchmark.sourcemeta.com.
///
/// ### Based on JSON Schema
///
/// JSON BinPack adopts the industry-standard schema language for JSON
/// documents, so the schemas you already have in your OpenAPI, AsyncAPI, RAML
/// or other specifications can be reused as they are. There is no additional
/// domain-specific schema language to learn, and no translation step between
/// them. For the same reason, JSON BinPack works alongside any schema evolution
/// tooling from the wider JSON Schema ecosystem.
///
/// ### Optional schema
///
/// JSON BinPack is a hybrid serialization format that runs in both
/// schema-driven and schema-less modes. Thanks to JSON Schema being a
/// constraint system rather than a structural definition language, a schema can
/// be as loose or as specific as the use case demands, even within the same
/// document, without reaching for a different serialization technology.
/// Free-form data and unknown fields encode without trouble as long as the
/// corresponding schema validates the data.
///
/// ### Space efficiency over runtime efficiency
///
/// When transmitting data over the Internet, time is the bottleneck, which
/// makes computation cheap in comparison. Leaving extremely low-powered devices
/// aside, trading more CPU cycles for better compression is a sensible choice.
/// JSON BinPack excels at improving network performance over unreliable and
/// low-bandwidth connections on mobile and IoT systems. If instead the use case
/// demands extreme runtime efficiency or zero-copy serialization, such as
/// intra-service or inter-process communication, consider a format such as
/// FlatBuffers or Cap'n Proto.
///
/// ### Research
///
/// JSON BinPack is the result of binary serialization research at the
/// Department of Computer Science of University of Oxford:
///
/// - [JSON BinPack: A space-efficient schema-driven and schema-less binary
///   serialization specification based on JSON
///   Schema](https://www.jviotti.com/dissertation.pdf)
/// - [Benchmarking JSON BinPack](https://arxiv.org/abs/2211.12799)
/// - [A benchmark of JSON-compatible binary serialization
///   specifications](https://arxiv.org/abs/2201.03051)
/// - [A survey of JSON-compatible binary serialization
///   specification](https://arxiv.org/abs/2201.02089)

namespace sourcemeta::blaze {

/// @ingroup binpack
///
/// Compile a JSON Schema into an encoding schema. Keep in mind this function
/// mutates the input schema. For example:
///
/// ```cpp
/// #include <sourcemeta/blaze/binpack.h>
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
///
/// #include <iostream>
/// #include <sstream>
///
/// auto schema{sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "integer",
///   "minimum": -100,
///   "maximum": 100
/// })JSON")};
///
/// sourcemeta::blaze::binpack_compile(
///     schema, sourcemeta::core::schema_walker,
///     sourcemeta::core::schema_resolver);
///
/// const auto encoding{sourcemeta::blaze::binpack_load(schema)};
///
/// std::ostringstream output;
/// sourcemeta::blaze::BinPackEncoder encoder{output};
/// encoder.write(sourcemeta::core::JSON{5}, encoding);
///
/// std::istringstream input{output.str()};
/// sourcemeta::blaze::BinPackDecoder decoder{input};
/// sourcemeta::core::prettify(decoder.read(encoding), std::cout);
/// std::cout << std::endl;
/// ```
SOURCEMETA_BLAZE_BINPACK_EXPORT
auto binpack_compile(sourcemeta::core::JSON &schema,
                     const sourcemeta::core::SchemaWalker &walker,
                     const sourcemeta::core::SchemaResolver &resolver,
                     std::string_view default_dialect = "") -> void;

/// @ingroup binpack
///
/// Transform a JSON Schema into its canonical form to prepare it for
/// compilation. Keep in mind this function mutates the input schema. Also, the
/// `binpack_compile` function already performs canonicalization. This function
/// is exposed mainly for debugging and testing purposes. For example:
///
/// ```cpp
/// #include <sourcemeta/blaze/binpack.h>
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
///
/// #include <iostream>
///
/// auto schema{sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "string"
/// })JSON")};
///
/// sourcemeta::blaze::binpack_canonicalize(
///     schema, sourcemeta::core::schema_walker,
///     sourcemeta::core::schema_resolver);
///
/// sourcemeta::core::prettify(schema, std::cout);
/// std::cout << std::endl;
/// ```
SOURCEMETA_BLAZE_BINPACK_EXPORT
auto binpack_canonicalize(sourcemeta::core::JSON &schema,
                          const sourcemeta::core::SchemaWalker &walker,
                          const sourcemeta::core::SchemaResolver &resolver,
                          std::string_view default_dialect = "") -> void;

/// @ingroup binpack
/// Load an encoding schema into its in-memory representation
SOURCEMETA_BLAZE_BINPACK_EXPORT
auto binpack_load(const sourcemeta::core::JSON &input) -> BinPackEncoding;

} // namespace sourcemeta::blaze

#endif
