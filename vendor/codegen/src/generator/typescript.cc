#include <sourcemeta/codegen/generator.h>

#include <iomanip> // std::hex, std::setfill, std::setw
#include <sstream> // std::ostringstream

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

namespace sourcemeta::codegen {

TypeScript::TypeScript(std::ostream &stream, const std::string_view type_prefix)
    : output{stream}, prefix{type_prefix} {}

auto TypeScript::operator()(const IRScalar &entry) -> void {
  this->output << "export type "
               << mangle(this->prefix, entry.pointer, entry.symbol, this->cache)
               << " = ";

  switch (entry.value) {
    case IRScalarType::String:
      this->output << "string";
      break;
    case IRScalarType::Number:
    case IRScalarType::Integer:
      this->output << "number";
      break;
    case IRScalarType::Boolean:
      this->output << "boolean";
      break;
    case IRScalarType::Null:
      this->output << "null";
      break;
  }

  this->output << ";\n";
}

auto TypeScript::operator()(const IREnumeration &entry) -> void {
  this->output << "export type "
               << mangle(this->prefix, entry.pointer, entry.symbol, this->cache)
               << " = ";

  const char *separator{""};
  for (const auto &value : entry.values) {
    this->output << separator;
    sourcemeta::core::prettify(value, this->output);
    separator = " | ";
  }

  this->output << ";\n";
}

auto TypeScript::operator()(const IRObject &entry) -> void {
  const auto type_name{
      mangle(this->prefix, entry.pointer, entry.symbol, this->cache)};
  const auto has_typed_additional{
      std::holds_alternative<IRType>(entry.additional)};
  const auto allows_any_additional{
      std::holds_alternative<bool>(entry.additional) &&
      std::get<bool>(entry.additional)};

  if (has_typed_additional && entry.members.empty()) {
    const auto &additional_type{std::get<IRType>(entry.additional)};
    this->output << "export type " << type_name << " = Record<string, "
                 << mangle(this->prefix, additional_type.pointer,
                           additional_type.symbol, this->cache)
                 << ">;\n";
    return;
  }

  if (allows_any_additional && entry.members.empty()) {
    this->output << "export type " << type_name
                 << " = Record<string, unknown>;\n";
    return;
  }

  this->output << "export interface " << type_name << " {\n";

  // We always quote property names for safety. JSON Schema allows any string
  // as a property name, but unquoted TypeScript/ECMAScript property names
  // must be valid IdentifierName productions (see ECMA-262 section 12.7).
  // Quoting allows any string to be used as a property name.
  // See: https://tc39.es/ecma262/#sec-names-and-keywords
  // See: https://mathiasbynens.be/notes/javascript-properties
  for (const auto &[member_name, member_value] : entry.members) {
    const auto optional_marker{member_value.required ? "" : "?"};
    const auto readonly_marker{member_value.immutable ? "readonly " : ""};

    this->output << "  " << readonly_marker << "\""
                 << escape_string(member_name) << "\"" << optional_marker
                 << ": "
                 << mangle(this->prefix, member_value.pointer,
                           member_value.symbol, this->cache)
                 << ";\n";
  }

  if (allows_any_additional) {
    this->output << "  [key: string]: unknown | undefined;\n";
  } else if (has_typed_additional) {
    // TypeScript index signatures must be a supertype of all property value
    // types. We use a union of all member types plus the additional properties
    // type plus undefined (for optional properties).
    this->output << "  [key: string]:\n";
    this->output << "    // As a notable limitation, TypeScript requires index "
                    "signatures\n";
    this->output << "    // to also include the types of all of its "
                    "properties, so we must\n";
    this->output << "    // match a superset of what JSON Schema allows\n";
    for (const auto &[member_name, member_value] : entry.members) {
      this->output << "    "
                   << mangle(this->prefix, member_value.pointer,
                             member_value.symbol, this->cache)
                   << " |\n";
    }

    const auto &additional_type{std::get<IRType>(entry.additional)};
    this->output << "    "
                 << mangle(this->prefix, additional_type.pointer,
                           additional_type.symbol, this->cache)
                 << " |\n";
    this->output << "    undefined;\n";
  }

  this->output << "}\n";
}

auto TypeScript::operator()(const IRImpossible &entry) -> void {
  this->output << "export type "
               << mangle(this->prefix, entry.pointer, entry.symbol, this->cache)
               << " = never;\n";
}

auto TypeScript::operator()(const IRAny &entry) -> void {
  this->output << "export type "
               << mangle(this->prefix, entry.pointer, entry.symbol, this->cache)
               << " = unknown;\n";
}

auto TypeScript::operator()(const IRArray &entry) -> void {
  this->output << "export type "
               << mangle(this->prefix, entry.pointer, entry.symbol, this->cache)
               << " = ";

  if (entry.items.has_value()) {
    this->output << mangle(this->prefix, entry.items->pointer,
                           entry.items->symbol, this->cache)
                 << "[]";
  } else {
    this->output << "unknown[]";
  }

  this->output << ";\n";
}

auto TypeScript::operator()(const IRReference &entry) -> void {
  this->output << "export type "
               << mangle(this->prefix, entry.pointer, entry.symbol, this->cache)
               << " = "
               << mangle(this->prefix, entry.target.pointer,
                         entry.target.symbol, this->cache)
               << ";\n";
}

auto TypeScript::operator()(const IRTuple &entry) -> void {
  this->output << "export type "
               << mangle(this->prefix, entry.pointer, entry.symbol, this->cache)
               << " = [";

  const char *separator{""};
  for (const auto &item : entry.items) {
    this->output << separator
                 << mangle(this->prefix, item.pointer, item.symbol,
                           this->cache);
    separator = ", ";
  }

  if (entry.additional.has_value()) {
    this->output << separator << "..."
                 << mangle(this->prefix, entry.additional->pointer,
                           entry.additional->symbol, this->cache)
                 << "[]";
  }

  this->output << "];\n";
}

auto TypeScript::operator()(const IRUnion &entry) -> void {
  this->output << "export type "
               << mangle(this->prefix, entry.pointer, entry.symbol, this->cache)
               << " =\n";

  const char *separator{""};
  for (const auto &value : entry.values) {
    this->output << separator << "  "
                 << mangle(this->prefix, value.pointer, value.symbol,
                           this->cache);
    separator = " |\n";
  }

  this->output << ";\n";
}

} // namespace sourcemeta::codegen
