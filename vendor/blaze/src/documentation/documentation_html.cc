#include <sourcemeta/blaze/documentation.h>

#include <sourcemeta/core/html.h>
#include <sourcemeta/core/json.h>

#include <cassert> // assert
#include <set>     // std::set
#include <sstream> // std::ostringstream
#include <string>  // std::string, std::to_string

namespace sourcemeta::blaze {

namespace {

auto json_to_string(const sourcemeta::core::JSON &value) -> std::string {
  std::ostringstream stream;
  sourcemeta::core::stringify(value, stream);
  return stream.str();
}

auto is_empty_row(const sourcemeta::core::JSON &row) -> bool {
  assert(row.is_object());
  assert(row.defines("type"));
  return row.at("type").defines("kind") &&
         row.at("type").at("kind").to_string() == "any" &&
         !row.defines("constraints") && !row.defines("badges") &&
         !row.defines("modifiers") && !row.defines("title") &&
         !row.defines("description") && !row.defines("default") &&
         !row.defines("examples");
}

auto collect_ref_targets(const sourcemeta::core::JSON &table,
                         std::set<std::int64_t> &targets) -> void {
  for (const auto &row : table.at("rows").as_array()) {
    if (row.defines("type") && row.at("type").defines("kind") &&
        row.at("type").at("kind").to_string() == "recursiveRef" &&
        row.at("type").defines("identifier")) {
      targets.insert(row.at("type").at("identifier").to_integer());
    }
    if (row.defines("children")) {
      for (const auto &section : row.at("children").as_array()) {
        for (const auto &child : section.at("children").as_array()) {
          collect_ref_targets(child, targets);
        }
      }
    }
  }
  if (table.defines("children")) {
    for (const auto &section : table.at("children").as_array()) {
      for (const auto &child : section.at("children").as_array()) {
        collect_ref_targets(child, targets);
      }
    }
  }
}

auto render_path(sourcemeta::core::HTMLWriter &writer,
                 const sourcemeta::core::JSON &path) -> void {
  assert(path.is_array());
  writer.code();
  bool first{true};
  for (const auto &segment : path.as_array()) {
    assert(segment.defines("type"));
    assert(segment.defines("value"));
    const auto &type{segment.at("type").to_string()};
    const auto &value{segment.at("value").to_string()};

    if (!first) {
      writer.text("/");
    }

    if (type == "literal" || type == "pattern") {
      writer.text(first ? "/" + value : value);
    } else if (type == "wildcard") {
      writer.text(first ? "/*" : "*");
    } else if (type == "synthetic") {
      writer.em("(" + value + ")");
    }

    first = false;
  }

  writer.close();
}

auto render_modifiers(sourcemeta::core::HTMLWriter &writer,
                      const sourcemeta::core::JSON &row) -> void {
  if (!row.defines("modifiers")) {
    return;
  }

  for (const auto &modifier : row.at("modifiers").as_array()) {
    writer.span(modifier.to_string());
  }
}

auto render_enum_values(sourcemeta::core::HTMLWriter &writer,
                        const sourcemeta::core::JSON &values,
                        const bool leading_separator) -> void {
  assert(values.is_array());
  bool first{true};
  for (const auto &value : values.as_array()) {
    if (!first || leading_separator) {
      writer.text(" | ");
    }

    writer.code(json_to_string(value));
    first = false;
  }
}

auto render_type_expression(sourcemeta::core::HTMLWriter &writer,
                            const sourcemeta::core::JSON &type) -> void {
  assert(type.is_object());
  assert(type.defines("kind"));
  const auto &kind{type.at("kind").to_string()};

  if (kind == "object") {
    writer.text("Object");
  } else if (kind == "primitive") {
    assert(type.defines("name"));
    const auto &name{type.at("name").to_string()};
    if (name == "string") {
      writer.text("String");
    } else if (name == "integer") {
      writer.text("Integer");
    } else if (name == "number") {
      writer.text("Number");
    }
  } else if (kind == "array" || kind == "tuple") {
    writer.text("Array");
  } else if (kind == "enum") {
    assert(type.defines("values"));
    render_enum_values(writer, type.at("values"), false);
    if (type.defines("overflow")) {
      writer.details();
      writer.summary("+ " + std::to_string(type.at("overflow").array_size()) +
                     " more");
      render_enum_values(writer, type.at("overflow"), true);
      writer.close();
    }
  } else if (kind == "externalRef") {
    assert(type.defines("url"));
    const auto &url{type.at("url").to_string()};
    writer.a().attribute("href", url);
    writer.text(url);
    writer.close();
  } else if (kind == "recursiveRef") {
    assert(type.defines("identifier"));
    const auto identifier{std::to_string(type.at("identifier").to_integer())};
    writer.a().attribute("data-index", identifier);
    if (type.defines("path")) {
      bool first_seg{true};
      for (const auto &segment : type.at("path").as_array()) {
        const auto &seg_type{segment.at("type").to_string()};
        const auto &seg_value{segment.at("value").to_string()};
        if (!first_seg) {
          writer.text("/");
        }
        if (seg_type == "synthetic") {
          writer.text("(" + seg_value + ")");
        } else if (seg_type == "literal" || seg_type == "pattern") {
          writer.text(first_seg ? "/" + seg_value : seg_value);
        } else if (seg_type == "wildcard") {
          writer.text(first_seg ? "/*" : "*");
        }
        first_seg = false;
      }
      writer.text(" #" + identifier);
    } else {
      writer.text(identifier);
    }
    writer.close();
  } else if (kind == "dynamicRef") {
    assert(type.defines("anchor"));
    writer.text("dynamic: " + type.at("anchor").to_string());
  } else if (kind == "any") {
    writer.text("Any");
  } else if (kind == "never") {
    writer.text("Never");
  }
}

auto render_badges(sourcemeta::core::HTMLWriter &writer,
                   const sourcemeta::core::JSON &row) -> void {
  if (!row.defines("badges")) {
    return;
  }

  for (const auto &badge : row.at("badges").as_array()) {
    assert(badge.defines("kind"));
    assert(badge.defines("value"));
    const auto &kind{badge.at("kind").to_string()};
    const auto &value{badge.at("value").to_string()};
    if (kind == "format") {
      writer.span(value);
    } else if (kind == "encoding") {
      writer.span("encoding: " + value);
    } else if (kind == "mime") {
      writer.span("mime: " + value);
    }
  }
}

auto render_notes(sourcemeta::core::HTMLWriter &writer,
                  const sourcemeta::core::JSON &row) -> void {
  if (row.defines("title")) {
    writer.strong(row.at("title").to_string());
  }

  if (row.defines("description")) {
    writer.p(row.at("description").to_string());
  }

  if (row.defines("default")) {
    writer.span();
    writer.text("default: ");
    writer.code(json_to_string(row.at("default")));
    writer.close();
  }
}

auto render_row(sourcemeta::core::HTMLWriter &writer,
                const sourcemeta::core::JSON &row,
                const std::set<std::int64_t> &ref_targets) -> void;
auto render_section(sourcemeta::core::HTMLWriter &writer,
                    const sourcemeta::core::JSON &section,
                    const std::set<std::int64_t> &ref_targets) -> void;
auto render_table(sourcemeta::core::HTMLWriter &writer,
                  const sourcemeta::core::JSON &table,
                  const std::set<std::int64_t> &ref_targets) -> void;

auto emit_header(sourcemeta::core::HTMLWriter &writer) -> void {
  writer.thead();
  writer.tr();
  writer.th("Path");
  writer.th("Type");
  writer.th("Required");
  writer.th("Constraints");
  writer.th("Notes");
  writer.close();
  writer.close();
}

auto render_row(sourcemeta::core::HTMLWriter &writer,
                const sourcemeta::core::JSON &row,
                const std::set<std::int64_t> &ref_targets) -> void {
  assert(row.defines("identifier"));
  assert(row.defines("path"));
  assert(row.defines("type"));

  const auto identifier{row.at("identifier").to_integer()};
  writer.tr().attribute("data-index", std::to_string(identifier));

  // Path
  writer.td();
  render_path(writer, row.at("path"));
  if (ref_targets.contains(identifier)) {
    writer.text(" ");
    writer.strong("#" + std::to_string(identifier));
  }
  render_modifiers(writer, row);
  writer.close();

  // Type
  writer.td();
  render_type_expression(writer, row.at("type"));
  render_badges(writer, row);
  writer.close();

  // Required
  writer.td();
  if (row.defines("required")) {
    writer.text(row.at("required").to_boolean() ? "Yes" : "No");
  }
  writer.close();

  // Constraints
  writer.td();
  if (row.defines("constraints")) {
    for (const auto &constraint : row.at("constraints").as_array()) {
      writer.span(constraint.to_string());
    }
  }
  writer.close();

  // Notes
  writer.td();
  render_notes(writer, row);
  writer.close();

  writer.close();

  if (row.defines("children")) {
    for (const auto &section : row.at("children").as_array()) {
      render_section(writer, section, ref_targets);
    }
  }
}

auto render_section(sourcemeta::core::HTMLWriter &writer,
                    const sourcemeta::core::JSON &section,
                    const std::set<std::int64_t> &ref_targets) -> void {
  assert(section.defines("label"));
  assert(section.defines("children"));

  writer.tr();
  writer.td().attribute("colspan", "5");
  writer.div();

  writer.div();
  writer.text(section.at("label").to_string());
  if (section.defines("position")) {
    writer.text(" ");
    writer.code(section.at("position").to_string());
  }
  writer.close();

  for (const auto &child : section.at("children").as_array()) {
    writer.div();
    if (child.defines("title")) {
      writer.div(child.at("title").to_string());
    }

    render_table(writer, child, ref_targets);
    writer.close();
  }

  writer.close();
  writer.close();
  writer.close();
}

auto render_table(sourcemeta::core::HTMLWriter &writer,
                  const sourcemeta::core::JSON &table,
                  const std::set<std::int64_t> &ref_targets) -> void {
  assert(table.defines("identifier"));
  assert(table.defines("rows"));

  writer.table().attribute("data-index",
                           std::to_string(table.at("identifier").to_integer()));

  const auto &rows{table.at("rows")};
  const auto has_children{table.defines("children")};
  const auto root_is_ref_target{
      !rows.empty() && rows.at(0).defines("identifier") &&
      ref_targets.contains(rows.at(0).at("identifier").to_integer())};
  const auto skip_root{has_children && !rows.empty() &&
                       is_empty_row(rows.at(0)) && !root_is_ref_target};

  if (!skip_root || rows.array_size() > 1) {
    emit_header(writer);
  }

  writer.tbody();
  for (std::size_t index = skip_root ? 1 : 0; index < rows.array_size();
       ++index) {
    render_row(writer, rows.at(index), ref_targets);
  }

  if (has_children) {
    for (const auto &section : table.at("children").as_array()) {
      render_section(writer, section, ref_targets);
    }
  }

  writer.close();
  writer.close();
}

} // namespace

auto to_html(const sourcemeta::core::JSON &documentation) -> std::string {
  assert(documentation.is_object());
  assert(documentation.defines("rows"));

  std::set<std::int64_t> ref_targets;
  collect_ref_targets(documentation, ref_targets);

  sourcemeta::core::HTMLWriter writer;
  writer.table().attribute("class", "sourcemeta-blaze-documentation");

  const auto &rows{documentation.at("rows")};
  const auto has_children{documentation.defines("children")};
  const auto root_is_ref_target{
      !rows.empty() && rows.at(0).defines("identifier") &&
      ref_targets.contains(rows.at(0).at("identifier").to_integer())};
  const auto skip_root{has_children && !rows.empty() &&
                       is_empty_row(rows.at(0)) && !root_is_ref_target};

  if (!skip_root || rows.array_size() > 1) {
    emit_header(writer);
  }

  writer.tbody();
  for (std::size_t index = skip_root ? 1 : 0; index < rows.array_size();
       ++index) {
    render_row(writer, rows.at(index), ref_targets);
  }

  if (has_children) {
    for (const auto &section : documentation.at("children").as_array()) {
      render_section(writer, section, ref_targets);
    }
  }

  writer.close();
  writer.close();
  return writer.str();
}

} // namespace sourcemeta::blaze
