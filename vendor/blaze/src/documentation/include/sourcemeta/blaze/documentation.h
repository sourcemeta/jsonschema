#ifndef SOURCEMETA_BLAZE_DOCUMENTATION_H_
#define SOURCEMETA_BLAZE_DOCUMENTATION_H_

/// @defgroup documentation Documentation
/// @brief Describe a JSON Schema as documentation, for readers
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/blaze/documentation.h>
/// ```
///
/// A schema is converted once into the table format, a description of data that
/// does not depend on any schema language, and a renderer reads that format
/// instead of the schema. The format states what a reader needs: which
/// properties there are, what may go in each, and which are required. Every
/// rule it states holds for the data the schema accepts, and a schema it cannot
/// describe exactly is refused.

#ifndef SOURCEMETA_BLAZE_DOCUMENTATION_EXPORT
#include <sourcemeta/blaze/documentation_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/blaze/documentation_error.h>
// NOLINTEND(misc-include-cleaner)

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::blaze {

/// @ingroup documentation
///
/// The address of the table format, which a document states as its `$schema`.
constexpr std::string_view DOCUMENTATION_FORMAT{
    "tag:sourcemeta.com,2026:table-format/2"};

/// @ingroup documentation
///
/// Describe a JSON Schema in the table format. For example:
///
/// ```cpp
/// #include <sourcemeta/blaze/documentation.h>
///
/// #include <sourcemeta/core/jsonschema.h>
/// #include <sourcemeta/core/json.h>
///
/// #include <cassert>
///
/// const sourcemeta::core::JSON schema =
///     sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "string"
/// })JSON");
///
/// const auto documentation{sourcemeta::blaze::to_documentation(
///     schema, sourcemeta::core::schema_walker,
///     sourcemeta::core::schema_resolver)};
///
/// assert(documentation.at("root").at("kind").to_string() == "string");
/// ```
///
/// Throws sourcemeta::blaze::DocumentationError when the schema cannot be
/// described exactly, saying what and where.
[[nodiscard]] SOURCEMETA_BLAZE_DOCUMENTATION_EXPORT auto
to_documentation(const sourcemeta::core::JSON &schema,
                 const sourcemeta::core::SchemaWalker &walker,
                 const sourcemeta::core::SchemaResolver &resolver,
                 const std::string_view default_dialect = "")
    -> sourcemeta::core::JSON;

/// @ingroup documentation
///
/// Render a table format document as HTML. The renderer reads the format alone,
/// so it draws a page for a schema of any language that was converted into it.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/blaze/documentation.h>
///
/// #include <sourcemeta/core/json.h>
///
/// #include <cassert>
///
/// const sourcemeta::core::JSON documentation =
///     sourcemeta::core::parse_json(R"JSON({
///   "$schema":
///   "tag:sourcemeta.com,2026:table-format/2",
///   "title": "Order",
///   "language": "https://json-schema.org/draft/2020-12/schema",
///   "root": { "kind": "string" }
/// })JSON");
///
/// std::string page;
/// sourcemeta::blaze::to_html(documentation, page);
/// assert(!page.empty());
/// ```
///
/// The HTML is appended to the given string. It carries no stylesheet: wrap it
/// in an element of your own and style it through its class names. A `pretty`
/// page puts every block on a line of its own, indented by how deep it is.
SOURCEMETA_BLAZE_DOCUMENTATION_EXPORT auto
to_html(const sourcemeta::core::JSON &documentation, std::string &output,
        const bool pretty = false) -> void;

} // namespace sourcemeta::blaze

#endif
