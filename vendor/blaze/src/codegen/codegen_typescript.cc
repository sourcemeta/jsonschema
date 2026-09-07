#include <sourcemeta/blaze/codegen.h>

#include <algorithm> // std::ranges::any_of
#include <iomanip>   // std::hex, std::setfill, std::setw
#include <sstream>   // std::ostringstream

namespace {

// TODO: Move to Core
auto escape_string(const std::string &input) -> std::string {
  std::ostringstream result;
  for (const auto character : input) {
    switch (character) {
      case '\\':
        result << "\\\\";
        break;
      case '"':
        result << "\\\"";
        break;
      case '\b':
        result << "\\b";
        break;
      case '\f':
        result << "\\f";
        break;
      case '\n':
        result << "\\n";
        break;
      case '\r':
        result << "\\r";
        break;
      case '\t':
        result << "\\t";
        break;
      default:
        // Escape other control characters (< 0x20) using \uXXXX format
        if (static_cast<unsigned char>(character) < 0x20) {
          result << "\\u" << std::hex << std::setfill('0') << std::setw(4)
                 << static_cast<int>(static_cast<unsigned char>(character));
        } else {
          result << character;
        }
        break;
    }
  }

  return result.str();
}

} // namespace

namespace sourcemeta::blaze {

TypeScript::TypeScript(std::ostream &stream, const std::string_view type_prefix)
    : output_{stream}, prefix_{type_prefix} {}

auto TypeScript::operator()(const CodegenIRScalar &entry) -> void {
  this->output_ << "export type "
                << mangle(this->prefix_, entry.pointer, entry.symbol,
                          this->cache_)
                << " = ";

  switch (entry.value) {
    case CodegenIRScalarType::String:
      this->output_ << "string";
      break;
    case CodegenIRScalarType::Number:
    case CodegenIRScalarType::Integer:
      this->output_ << "number";
      break;
    case CodegenIRScalarType::Boolean:
      this->output_ << "boolean";
      break;
    case CodegenIRScalarType::Null:
      this->output_ << "null";
      break;
  }

  this->output_ << ";\n";
}

auto TypeScript::operator()(const CodegenIREnumeration &entry) -> void {
  this->output_ << "export type "
                << mangle(this->prefix_, entry.pointer, entry.symbol,
                          this->cache_)
                << " = ";

  const char *separator{""};
  for (const auto &value : entry.values) {
    this->output_ << separator;
    sourcemeta::core::prettify(value, this->output_);
    separator = " | ";
  }

  this->output_ << ";\n";
}

auto TypeScript::operator()(const CodegenIRObject &entry) -> void {
  const auto type_name{
      mangle(this->prefix_, entry.pointer, entry.symbol, this->cache_)};
  const auto has_typed_additional{
      std::holds_alternative<CodegenIRType>(entry.additional)};
  const auto allows_any_additional{
      std::holds_alternative<bool>(entry.additional) &&
      std::get<bool>(entry.additional)};

  if (has_typed_additional && entry.members.empty() && entry.pattern.empty()) {
    const auto &additional_type{std::get<CodegenIRType>(entry.additional)};
    this->output_ << "export type " << type_name << " = Record<string, "
                  << mangle(this->prefix_, additional_type.pointer,
                            additional_type.symbol, this->cache_)
                  << ">;\n";
    return;
  }

  if (allows_any_additional && entry.members.empty() && entry.pattern.empty()) {
    this->output_ << "export type " << type_name
                  << " = Record<string, unknown>;\n";
    return;
  }

  this->output_ << "export interface " << type_name << " {\n";

  // We always quote property names for safety. JSON Schema allows any string
  // as a property name, but unquoted TypeScript/ECMAScript property names
  // must be valid IdentifierName productions (see ECMA-262 section 12.7).
  // Quoting allows any string to be used as a property name.
  // See: https://tc39.es/ecma262/#sec-names-and-keywords
  // See: https://mathiasbynens.be/notes/javascript-properties
  for (const auto &[member_name, member_value] : entry.members) {
    const auto *const optional_marker{member_value.required ? "" : "?"};
    const auto *const readonly_marker{member_value.immutable ? "readonly "
                                                             : ""};

    this->output_ << "  " << readonly_marker << "\""
                  << escape_string(member_name) << "\"" << optional_marker
                  << ": "
                  << mangle(this->prefix_, member_value.pointer,
                            member_value.symbol, this->cache_)
                  << ";\n";
  }

  for (const auto &pattern_property : entry.pattern) {
    if (!pattern_property.prefix.has_value()) {
      continue;
    }

    this->output_ << "  [key: `" << pattern_property.prefix.value()
                  << "${string}`]: "
                  << mangle(this->prefix_, pattern_property.pointer,
                            pattern_property.symbol, this->cache_);

    // TypeScript requires that a more specific index signature type is
    // assignable to any less specific one that overlaps it. When a prefix
    // is a sub-prefix of another (i.e. "x-data-" starts with "x-"),
    // intersect the types so the constraint is satisfied
    for (const auto &other : entry.pattern) {
      if (&other == &pattern_property || !other.prefix.has_value()) {
        continue;
      }

      if (pattern_property.prefix.value().starts_with(other.prefix.value())) {
        this->output_ << " & "
                      << mangle(this->prefix_, other.pointer, other.symbol,
                                this->cache_);
      }
    }

    this->output_ << ";\n";
  }

  const auto has_non_prefix_pattern{std::ranges::any_of(
      entry.pattern, [](const auto &pattern_property) -> auto {
        return !pattern_property.prefix.has_value();
      })};

  if (allows_any_additional) {
    this->output_ << "  [key: string]: unknown | undefined;\n";
  } else if (has_typed_additional || has_non_prefix_pattern) {
    // TypeScript index signatures must be a supertype of all property value
    // types. We use a union of all member types plus the additional properties
    // type plus undefined (for optional properties).
    this->output_ << "  [key: string]:\n";
    this->output_
        << "    // As a notable limitation, TypeScript requires index "
           "signatures\n";
    this->output_ << "    // to also include the types of all of its "
                     "properties, so we must\n";
    this->output_ << "    // match a superset of what JSON Schema allows\n";
    for (const auto &[member_name, member_value] : entry.members) {
      this->output_ << "    "
                    << mangle(this->prefix_, member_value.pointer,
                              member_value.symbol, this->cache_)
                    << " |\n";
    }

    for (const auto &pattern_property : entry.pattern) {
      this->output_ << "    "
                    << mangle(this->prefix_, pattern_property.pointer,
                              pattern_property.symbol, this->cache_)
                    << " |\n";
    }

    if (has_typed_additional) {
      const auto &additional_type{std::get<CodegenIRType>(entry.additional)};
      this->output_ << "    "
                    << mangle(this->prefix_, additional_type.pointer,
                              additional_type.symbol, this->cache_)
                    << " |\n";
    }

    this->output_ << "    undefined;\n";
  }

  this->output_ << "}\n";
}

auto TypeScript::operator()(const CodegenIRImpossible &entry) -> void {
  this->output_ << "export type "
                << mangle(this->prefix_, entry.pointer, entry.symbol,
                          this->cache_)
                << " = never;\n";
}

auto TypeScript::operator()(const CodegenIRAny &entry) -> void {
  this->output_ << "export type "
                << mangle(this->prefix_, entry.pointer, entry.symbol,
                          this->cache_)
                << " = unknown;\n";
}

auto TypeScript::operator()(const CodegenIRArray &entry) -> void {
  this->output_ << "export type "
                << mangle(this->prefix_, entry.pointer, entry.symbol,
                          this->cache_)
                << " = ";

  if (entry.items.has_value()) {
    this->output_ << mangle(this->prefix_, entry.items->pointer,
                            entry.items->symbol, this->cache_)
                  << "[]";
  } else {
    this->output_ << "unknown[]";
  }

  this->output_ << ";\n";
}

auto TypeScript::operator()(const CodegenIRReference &entry) -> void {
  this->output_ << "export type "
                << mangle(this->prefix_, entry.pointer, entry.symbol,
                          this->cache_)
                << " = "
                << mangle(this->prefix_, entry.target.pointer,
                          entry.target.symbol, this->cache_)
                << ";\n";
}

auto TypeScript::operator()(const CodegenIRTuple &entry) -> void {
  this->output_ << "export type "
                << mangle(this->prefix_, entry.pointer, entry.symbol,
                          this->cache_)
                << " = [";

  const char *separator{""};
  for (const auto &item : entry.items) {
    this->output_ << separator
                  << mangle(this->prefix_, item.pointer, item.symbol,
                            this->cache_);
    separator = ", ";
  }

  if (entry.additional.has_value()) {
    this->output_ << separator << "..."
                  << mangle(this->prefix_, entry.additional->pointer,
                            entry.additional->symbol, this->cache_)
                  << "[]";
  }

  this->output_ << "];\n";
}

auto TypeScript::operator()(const CodegenIRUnion &entry) -> void {
  this->output_ << "export type "
                << mangle(this->prefix_, entry.pointer, entry.symbol,
                          this->cache_)
                << " =\n";

  const char *separator{""};
  for (const auto &value : entry.values) {
    this->output_ << separator << "  "
                  << mangle(this->prefix_, value.pointer, value.symbol,
                            this->cache_);
    separator = " |\n";
  }

  this->output_ << ";\n";
}

auto TypeScript::operator()(const CodegenIRIntersection &entry) -> void {
  this->output_ << "export type "
                << mangle(this->prefix_, entry.pointer, entry.symbol,
                          this->cache_)
                << " =\n";

  const char *separator{""};
  for (const auto &value : entry.values) {
    this->output_ << separator << "  "
                  << mangle(this->prefix_, value.pointer, value.symbol,
                            this->cache_);
    separator = " &\n";
  }

  this->output_ << ";\n";
}

auto TypeScript::operator()(const CodegenIRConditional &entry) -> void {
  // As a notable limitation, TypeScript cannot express the negation of an
  // if/then/else condition, so the else branch is wider than what JSON
  // Schema allows
  this->output_ << "// (if & then) | else approximation: the else branch is "
                   "wider than what\n";
  this->output_ << "// JSON Schema allows, as TypeScript cannot express type "
                   "negation\n";
  this->output_ << "export type "
                << mangle(this->prefix_, entry.pointer, entry.symbol,
                          this->cache_)
                << " =\n  ("
                << mangle(this->prefix_, entry.condition.pointer,
                          entry.condition.symbol, this->cache_)
                << " & "
                << mangle(this->prefix_, entry.consequent.pointer,
                          entry.consequent.symbol, this->cache_)
                << ") | "
                << mangle(this->prefix_, entry.alternative.pointer,
                          entry.alternative.symbol, this->cache_)
                << ";\n";
}

} // namespace sourcemeta::blaze
