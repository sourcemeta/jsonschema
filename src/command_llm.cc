#include <sourcemeta/core/http.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/options.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/convert.h>
#include <sourcemeta/blaze/editor.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

#include <algorithm>   // std::ranges::all_of
#include <array>       // std::array
#include <cassert>     // assert
#include <cctype>      // std::isdigit
#include <chrono>      // std::chrono::seconds
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint64_t
#include <deque>       // std::deque
#include <filesystem>  // std::filesystem
#include <functional>  // std::ref
#include <iostream>    // std::cout, std::cerr
#include <iterator>    // std::prev
#include <memory>      // std::make_shared
#include <optional>    // std::optional
#include <ostream>     // std::ostream
#include <sstream>     // std::istringstream, std::ostringstream
#include <stdexcept>   // std::out_of_range
#include <string>      // std::string, std::stoull
#include <string_view> // std::string_view
#include <utility>     // std::move, std::pair

#include "command.h"
#include "configuration.h"
#include "error.h"
#include "input.h"
#include "logger.h"
#include "print.h"
#include "resolver.h"
#include "utils.h"

namespace {

// Providers read this as a label and as nothing else. It is fixed rather than
// taken from the file name because OpenAI constrains it to
// `^[a-zA-Z0-9_-]{1,64}$`, which a file name is under no obligation to satisfy,
// and a request turned down over the name of the schema rather than over the
// schema itself is the one finding this command must never manufacture
constexpr std::string_view SCHEMA_NAME{"schema"};

// A completion response runs to kilobytes. A model asked for JSON without being
// told to stop has been documented to emit whitespace until it reaches the
// token limit, so there is a ceiling rather than none
constexpr std::size_t MAXIMUM_RESPONSE_SIZE{std::size_t{16} * 1024 * 1024};

// Compiling a grammar out of a schema takes a provider well past what an
// ordinary request takes, and a schema that makes it pathological has been seen
// to run beyond a hundred seconds, so this starts above the HTTP default
constexpr std::uint64_t DEFAULT_TIMEOUT_SECONDS{120};

// The names a provider reports token counts under, and what they are reported
// as
struct UsageField {
  std::string_view wire;
  std::string_view reported;
  std::string_view label;
};

constexpr std::array<UsageField, 3> USAGE_FIELDS{
    {{.wire = "prompt_tokens", .reported = "promptTokens", .label = "prompt"},
     {.wire = "completion_tokens",
      .reported = "completionTokens",
      .label = "completion"},
     {.wire = "total_tokens", .reported = "totalTokens", .label = "total"}}};

// The OpenAPI Schema Object dialects are deliberately absent. They are 2020-12
// plus a vocabulary that describes an API rather than an instance, so no
// provider recognises one, and spelling a schema that way for one would be a
// setting with no use
auto parse_upgrade_target(const std::string_view value)
    -> sourcemeta::blaze::ConvertTarget {
  if (value == "draft4") {
    return sourcemeta::blaze::ConvertTarget::Draft4;
  }

  if (value == "draft6") {
    return sourcemeta::blaze::ConvertTarget::Draft6;
  }

  if (value == "draft7") {
    return sourcemeta::blaze::ConvertTarget::Draft7;
  }

  if (value == "2019-09") {
    return sourcemeta::blaze::ConvertTarget::Draft201909;
  }

  if (value == "2020-12") {
    return sourcemeta::blaze::ConvertTarget::Draft202012;
  }

  throw sourcemeta::jsonschema::InvalidOptionEnumerationValueError{
      "The given target dialect is not supported",
      "upgrade",
      {"draft4", "draft6", "draft7", "2019-09", "2020-12"}};
}

auto parse_timeout(const sourcemeta::core::Options &options)
    -> std::chrono::seconds {
  if (!options.contains("timeout")) {
    return std::chrono::seconds{DEFAULT_TIMEOUT_SECONDS};
  }

  const std::string value{options.at("timeout").front()};
  if (value.empty() || !std::ranges::all_of(value, [](const char character) {
        return std::isdigit(static_cast<unsigned char>(character));
      })) {
    throw sourcemeta::jsonschema::InvalidTimeoutError{};
  }

  std::uint64_t seconds{0};
  try {
    seconds = std::stoull(value);
  } catch (const std::out_of_range &) {
    throw sourcemeta::jsonschema::InvalidTimeoutError{};
  }

  if (seconds == 0) {
    throw sourcemeta::jsonschema::InvalidTimeoutError{};
  }

  return std::chrono::seconds{seconds};
}

// A value is read as JSON so that `=true` is a boolean and `=2048` a number,
// falling back to the text as written. Without the fallback a model name would
// have to be quoted twice over at the shell, and without the reading
// `strict=false` would arrive as a string, which is truthy to most consumers.
// Anything trailing the first value means the text was never JSON to begin with
auto parse_parameter_value(const std::string_view value)
    -> sourcemeta::core::JSON {
  const std::string input{value};
  std::istringstream stream{input};

  try {
    auto result{sourcemeta::core::parse_json(stream)};
    if (sourcemeta::jsonschema::at_end_of_stream(input, stream)) {
      return result;
    }
  } catch (const sourcemeta::core::JSONParseError &) {
    return sourcemeta::core::JSON{input};
  }

  return sourcemeta::core::JSON{input};
}

// Where the schema being tested sits in the request body
constexpr std::array<std::string_view, 3> SCHEMA_LOCATION{
    "response_format", "json_schema", "schema"};

// How a parameter stands in relation to the schema being sent. The response is
// checked against the schema that was read, so neither rewriting it nor
// replacing what carries it can be allowed, but they are different mistakes and
// worth saying apart
enum class SchemaReach : std::uint8_t { Elsewhere, TheSchema, WhatCarriesIt };

auto schema_reach(const sourcemeta::core::Pointer &pointer) -> SchemaReach {
  const auto depth{std::min(pointer.size(), SCHEMA_LOCATION.size())};
  for (std::size_t index = 0; index < depth; index++) {
    const auto &token{pointer.at(index)};
    if (!token.is_property() || token.to_property() != SCHEMA_LOCATION[index]) {
      return SchemaReach::Elsewhere;
    }
  }

  return pointer.size() < SCHEMA_LOCATION.size() ? SchemaReach::WhatCarriesIt
                                                 : SchemaReach::TheSchema;
}

// Where a parameter writes, creating the objects along the way, as a request
// body has no reason to already hold the shape a provider-specific setting sits
// under. An array position is never created, as JSON holds no holes
auto parameter_target(sourcemeta::core::JSON &body,
                      const sourcemeta::core::Pointer &pointer,
                      const std::string &entry) -> sourcemeta::core::JSON * {
  auto *current{&body};
  for (auto token{pointer.cbegin()}; token != std::prev(pointer.cend());
       ++token) {
    if (token->is_property()) {
      if (!current->is_object()) {
        throw sourcemeta::jsonschema::InvalidParameterError{
            "The parameter writes through a value that is not an object",
            entry};
      }

      if (!current->defines(token->to_property())) {
        current->assign(token->to_property(),
                        sourcemeta::core::JSON::make_object());
      }

      current = &current->at(token->to_property());
      continue;
    }

    if (token->is_hyphen()) {
      throw sourcemeta::jsonschema::InvalidParameterError{
          "The parameter may only append at the end of its location", entry};
    }

    if (!current->is_array() || token->to_index() >= current->size()) {
      throw sourcemeta::jsonschema::InvalidParameterError{
          "The parameter writes through an array position that does not exist",
          entry};
    }

    current = &current->at(token->to_index());
  }

  return current;
}

auto overlay_parameter(sourcemeta::core::JSON &body,
                       const std::string_view parameter) -> void {
  const std::string entry{parameter};
  const auto separator{entry.find('=')};
  if (separator == std::string::npos) {
    throw sourcemeta::jsonschema::MalformedParameterError{
        "Parameters must be in the form `<pointer>=<value>`", entry};
  }

  const auto location{entry.substr(0, separator)};
  if (location.empty()) {
    throw sourcemeta::jsonschema::MalformedParameterError{
        "The parameter must name a location within the request body", entry};
  }

  if (!sourcemeta::core::is_pointer(location)) {
    throw sourcemeta::jsonschema::MalformedParameterError{
        "The parameter location must be a JSON Pointer", entry};
  }

  const auto pointer{sourcemeta::core::to_pointer(location)};
  switch (schema_reach(pointer)) {
    case SchemaReach::TheSchema:
      throw sourcemeta::jsonschema::InvalidParameterError{
          "A parameter cannot write the schema, as that is what the response "
          "is checked against",
          entry};
    case SchemaReach::WhatCarriesIt:
      throw sourcemeta::jsonschema::InvalidParameterError{
          "A parameter cannot replace the part of the request that carries "
          "the schema",
          entry};
    case SchemaReach::Elsewhere:
      break;
  }

  auto value{
      parse_parameter_value(std::string_view{entry}.substr(separator + 1))};
  auto *target{parameter_target(body, pointer, entry)};
  const auto &last{pointer.back()};

  if (last.is_property()) {
    if (!target->is_object()) {
      throw sourcemeta::jsonschema::InvalidParameterError{
          "The parameter writes a property into a value that is not an object",
          entry};
    }

    target->assign(last.to_property(), std::move(value));
    return;
  }

  if (!target->is_array()) {
    throw sourcemeta::jsonschema::InvalidParameterError{
        "The parameter writes an array position into a value that is not an "
        "array",
        entry};
  }

  if (last.is_hyphen()) {
    target->push_back(std::move(value));
    return;
  }

  if (last.to_index() >= target->size()) {
    throw sourcemeta::jsonschema::InvalidParameterError{
        "The parameter writes an array position that does not exist", entry};
  }

  target->at(last.to_index()).into(std::move(value));
}

auto make_request_body(const sourcemeta::core::JSON &schema,
                       const std::string_view model,
                       const std::string_view prompt)
    -> sourcemeta::core::JSON {
  auto message{sourcemeta::core::JSON::make_object()};
  message.assign("role", sourcemeta::core::JSON{"user"});
  message.assign("content", sourcemeta::core::JSON{std::string{prompt}});

  auto messages{sourcemeta::core::JSON::make_array()};
  messages.push_back(std::move(message));

  auto json_schema{sourcemeta::core::JSON::make_object()};
  json_schema.assign("name", sourcemeta::core::JSON{std::string{SCHEMA_NAME}});
  // Structured Outputs does not engage at all without this, so it sits in the
  // template rather than being left to the caller to remember
  json_schema.assign("strict", sourcemeta::core::JSON{true});
  json_schema.assign("schema", schema);

  auto response_format{sourcemeta::core::JSON::make_object()};
  response_format.assign("type", sourcemeta::core::JSON{"json_schema"});
  response_format.assign("json_schema", std::move(json_schema));

  // Nothing else goes out by default. Every sampling setting is named
  // differently by each provider and several refuse a non-default value, so the
  // portable request omits them and `--param` adds what a given endpoint wants
  auto body{sourcemeta::core::JSON::make_object()};
  body.assign("model", sourcemeta::core::JSON{std::string{model}});
  body.assign("messages", std::move(messages));
  body.assign("response_format", std::move(response_format));
  return body;
}

// The request as it would go out, headers and all. A value is printed as it was
// given so that what comes out can be replayed by whatever else speaks HTTP,
// which means a credential passed here lands in the output and should be
// treated the way the credential itself is
auto print_dry_run(const sourcemeta::core::Options &options,
                   const std::string_view url,
                   const sourcemeta::core::JSON &body) -> void {
  const auto headers{sourcemeta::jsonschema::collect_http_headers(options)};

  if (options.contains("json")) {
    auto entries{sourcemeta::core::JSON::make_array()};
    auto content_type{sourcemeta::core::JSON::make_object()};
    content_type.assign("name", sourcemeta::core::JSON{"Content-Type"});
    content_type.assign("value", sourcemeta::core::JSON{"application/json"});
    entries.push_back(std::move(content_type));

    for (const auto &header : headers) {
      auto entry{sourcemeta::core::JSON::make_object()};
      entry.assign("name", sourcemeta::core::JSON{std::string{header.first}});
      entry.assign("value", sourcemeta::core::JSON{std::string{header.second}});
      entries.push_back(std::move(entry));
    }

    auto request{sourcemeta::core::JSON::make_object()};
    request.assign("method", sourcemeta::core::JSON{"POST"});
    request.assign("url", sourcemeta::core::JSON{std::string{url}});
    request.assign("headers", std::move(entries));
    request.assign("body", body);
    sourcemeta::core::prettify(request, std::cout);
    std::cout << "\n";
    return;
  }

  std::cout << "POST " << url << "\n";
  std::cout << "Content-Type: application/json\n";
  for (const auto &header : headers) {
    std::cout << header.first << ": " << header.second << "\n";
  }

  std::cout << "\n";
  sourcemeta::core::prettify(body, std::cout);
  std::cout << "\n";
}

// The model's output as it came. A reformatted copy would shift every
// coordinate reported against it and could hide behaviour that turns on
// whitespace, so what gets printed is the evidence. One newline is added when
// the output does not end in one, so that whatever follows starts on its own
// line
auto print_verbatim(const std::string_view value, std::ostream &stream)
    -> void {
  stream << value;
  if (!value.ends_with('\n')) {
    stream << "\n";
  }
}

auto is_successful(const sourcemeta::core::HTTPStatus &status) -> bool {
  return status.code >= 200 && status.code < 300;
}

// What an unsuccessful response says about whose problem it is. A schema the
// provider refused is the finding this command exists to produce, a credential
// or a URL that does not work is the invocation being wrong, and a provider
// that is unwell or busy is neither
auto exit_code_for_status(const sourcemeta::core::HTTPStatus &status) -> int {
  switch (status.code) {
    case 400:
    case 413:
    case 422:
      return sourcemeta::jsonschema::EXIT_NOT_SUPPORTED;
    case 429:
      return sourcemeta::jsonschema::EXIT_UNEXPECTED_ERROR;
    default:
      break;
  }

  if (status.code >= 500) {
    return sourcemeta::jsonschema::EXIT_UNEXPECTED_ERROR;
  }

  return sourcemeta::jsonschema::EXIT_OTHER_INPUT_ERROR;
}

// Error bodies are not portable, not even within one provider, and one of the
// shapes seen in the wild is not JSON at all. Nothing is read out of one or
// summarised from it, so what came back is what is shown, laid out when it
// happens to be JSON and left exactly as it arrived when it is not
auto report_response_body(const sourcemeta::core::Options &options,
                          const std::string_view message,
                          const std::string_view url,
                          const sourcemeta::core::HTTPResponse &response)
    -> void {
  if (options.contains("json")) {
    auto result{sourcemeta::core::JSON::make_object()};
    result.assign("error", sourcemeta::core::JSON{std::string{message}});
    result.assign("status", sourcemeta::core::JSON{static_cast<std::size_t>(
                                response.status.code)});
    result.assign("url", sourcemeta::core::JSON{std::string{url}});
    result.assign("body", sourcemeta::core::JSON{response.body});
    sourcemeta::core::prettify(result, std::cout);
    std::cout << "\n";
    return;
  }

  std::cerr << "error: " << message << "\n";
  std::cerr << "  with status ";
  if (response.status.wire.empty()) {
    std::cerr << response.status.code;
  } else {
    std::cerr << response.status.wire;
  }

  std::cerr << "\n";
  std::cerr << "  at url " << url << "\n";
  if (response.body.empty()) {
    return;
  }

  // Whose words these are is worth saying outright, as an error body reads like
  // one of ours otherwise and is written to none of our conventions
  std::cerr << "\nThe provider responded with:\n\n";
  try {
    const auto parsed{sourcemeta::core::parse_json(response.body)};
    sourcemeta::core::prettify(parsed, std::cerr);
    std::cerr << "\n";
  } catch (const sourcemeta::core::JSONParseError &) {
    print_verbatim(response.body, std::cerr);
  }
}

// A choice is where a generated document and how generation stopped both sit,
// so reaching either means getting this far first
auto first_choice(const sourcemeta::core::JSON &envelope)
    -> const sourcemeta::core::JSON * {
  if (!envelope.is_object()) {
    return nullptr;
  }

  const auto *choices{envelope.try_at("choices")};
  if (choices == nullptr || !choices->is_array() || choices->empty()) {
    return nullptr;
  }

  const auto &choice{choices->at(std::size_t{0})};
  return choice.is_object() ? &choice : nullptr;
}

// The generated document arrives as a string nested in the envelope. Exactly
// one place is read, as a reasoning model puts its chain of thought in
// `reasoning_content` right beside it and that is never the answer
auto generated_content(const sourcemeta::core::JSON &envelope)
    -> const sourcemeta::core::JSON * {
  const auto *choice{first_choice(envelope)};
  if (choice == nullptr) {
    return nullptr;
  }

  const auto *message{choice->try_at("message")};
  if (message == nullptr || !message->is_object()) {
    return nullptr;
  }

  const auto *content{message->try_at("content")};
  return content != nullptr && content->is_string() ? content : nullptr;
}

auto finish_reason(const sourcemeta::core::JSON &envelope)
    -> const sourcemeta::core::JSON * {
  const auto *choice{first_choice(envelope)};
  if (choice == nullptr) {
    return nullptr;
  }

  const auto *reason{choice->try_at("finish_reason")};
  return reason != nullptr && reason->is_string() ? reason : nullptr;
}

// Token counts are read where a provider happens to report them and skipped
// where it does not. The validation result is the point, and a count never
// decides it, so a block we cannot read must never fail the command
auto usage_json(const sourcemeta::core::JSON &envelope)
    -> std::optional<sourcemeta::core::JSON> {
  if (!envelope.is_object()) {
    return std::nullopt;
  }

  const auto *usage{envelope.try_at("usage")};
  if (usage == nullptr || !usage->is_object()) {
    return std::nullopt;
  }

  auto result{sourcemeta::core::JSON::make_object()};
  for (const auto &field : USAGE_FIELDS) {
    const auto *value{usage->try_at(field.wire)};
    if (value != nullptr && value->is_integer()) {
      result.assign(sourcemeta::core::JSON::String{field.reported}, *value);
    }
  }

  if (result.empty()) {
    return std::nullopt;
  }

  return result;
}

// What the provider said it spent, where it said anything at all. One that
// keeps its counts to itself leaves nothing to report rather than a line of
// blanks
auto report_usage(const sourcemeta::core::JSON &envelope) -> void {
  const auto usage{usage_json(envelope)};
  if (!usage.has_value()) {
    return;
  }

  std::cerr << "\ntokens:";
  bool first{true};
  for (const auto &field : USAGE_FIELDS) {
    const auto *value{usage.value().try_at(field.reported)};
    if (value == nullptr) {
      continue;
    }

    std::cerr << (first ? " " : ", ");
    first = false;
    sourcemeta::core::stringify(*value, std::cerr);
    std::cerr << " " << field.label;
  }

  std::cerr << "\n";
}

// Spelling a schema for a provider is the same rewrite an upgrade performs, but
// a custom meta-schema means something else here than it does to somebody
// upgrading a schema on purpose: not a rewrite we cannot do, but a dialect the
// other end could never have known about. So that one is reported in these
// terms and the rest travel as they are
auto upgrade_for_wire(sourcemeta::core::JSON &wire,
                      const sourcemeta::core::SchemaResolver &resolver,
                      const sourcemeta::blaze::ConvertTarget target,
                      const std::string &dialect, const std::string &default_id,
                      const std::filesystem::path &schema_display_path,
                      const sourcemeta::core::PointerPositionTracker &positions)
    -> void {
  try {
    sourcemeta::jsonschema::upgrade_schema(wire, resolver, target, dialect,
                                           default_id, schema_display_path,
                                           positions);
  } catch (const sourcemeta::jsonschema::PositionError<
           sourcemeta::jsonschema::CustomMetaschemaUpgradeError> &error) {
    throw sourcemeta::jsonschema::PositionError<
        sourcemeta::jsonschema::CustomMetaschemaLLMError>{
        error.line(), error.column(), error.path(), error.location(),
        error.uri()};
  } catch (const sourcemeta::jsonschema::CustomMetaschemaUpgradeError &error) {
    throw sourcemeta::jsonschema::CustomMetaschemaLLMError{
        error.path(), error.location(), error.uri()};
  }
}

} // namespace

auto sourcemeta::jsonschema::llm(const sourcemeta::core::Options &options)
    -> void {
  if (options.positional().empty()) {
    throw PositionalArgumentError{
        "This command expects a path to a schema",
        "jsonschema llm path/to/schema.json --ask \"What is the capital of "
        "Germany?\" --url https://api.example.com/v1/chat/completions --model "
        "my-model"};
  }

  // Remote resolution sends the headers this command was given, the credential
  // for the completion request among them, to whatever host a reference names.
  // So references resolve locally here and the rest composes through `bundle`
  if (options.contains("http")) {
    throw OptionConflictError{
        "The `--http/-h` option is not supported by this command. Run `bundle "
        "--http` first and pass its output to this command instead"};
  }

  validate_http_headers(options);

  if (!options.contains("ask") || options.at("ask").front().empty()) {
    throw MissingOptionError{
        "You must pass a prompt using the `--ask/-a` option",
        "--ask \"What is the capital of Germany?\""};
  }

  if (!options.contains("url") || options.at("url").front().empty()) {
    throw MissingOptionError{
        "You must pass the full URL of the structured output endpoint using "
        "the `--url/-u` option",
        "--url https://api.example.com/v1/chat/completions"};
  }

  if (!options.contains("model") || options.at("model").front().empty()) {
    throw MissingOptionError{"You must pass the model using the `--model/-m` "
                             "option",
                             "--model my-model"};
  }

  const auto timeout{parse_timeout(options)};
  const auto upgrade_target{
      options.contains("upgrade")
          ? parse_upgrade_target(options.at("upgrade").front())
          : sourcemeta::blaze::ConvertTarget::Draft202012};

  const std::filesystem::path schema_path{options.positional().front()};
  const bool schema_from_stdin = (schema_path == "-");

  check_no_duplicate_stdin(options.positional());

  if (!schema_from_stdin && std::filesystem::is_directory(schema_path)) {
    throw sourcemeta::core::IOIsADirectoryError{schema_path};
  }

  const auto schema_config_base{
      schema_from_stdin ? std::filesystem::current_path() : schema_path};
  const auto configuration_path{
      find_configuration(options, schema_config_base)};
  const auto &configuration{
      read_configuration(options, configuration_path, schema_config_base)};
  const auto dialect{default_dialect(options, configuration)};

  // The input format is never written back out, as what this command prints is
  // the model's answer rather than the schema, so nothing is kept to write with
  auto parsed_schema{schema_from_stdin ? read_from_stdin()
                                       : read_file(schema_path)};

  const auto display_path{schema_from_stdin ? stdin_path() : schema_path};

  if (parsed_schema.multidocument) {
    throw MultiDocumentInputError{
        "This command does not support input with multiple documents",
        display_path};
  }

  // A description holds many schemas and none of itself, and what a provider
  // takes is one schema that stands on its own
  if (is_openapi_document(parsed_schema.document)) {
    throw sourcemeta::core::FileError<UnsupportedOpenAPILLMError>(
        schema_from_stdin ? openapi_stdin_path() : schema_path);
  }

  if (!parsed_schema.document.is_object() &&
      !parsed_schema.document.is_boolean()) {
    throw NotSchemaError{display_path};
  }

  // A boolean schema says the same thing to every document it is handed, so
  // asking a model to satisfy one asks nothing, and there is nothing in the
  // answer to find out about either
  if (parsed_schema.document.is_boolean()) {
    throw sourcemeta::core::FileError<BooleanSchemaLLMError>(display_path);
  }

  const auto &custom_resolver{resolver(options, false, dialect, configuration)};
  const auto schema_default_id{
      sourcemeta::jsonschema::default_id(schema_path, schema_from_stdin)};

  // Bundling inlines what the schema references so that nothing has to be
  // reached for later, which is at once what a provider needs to be handed one
  // self-contained document and what Blaze needs to evaluate anything at all.
  // It changes nothing about what the schema asserts, so one bundle serves both
  const auto bundled{bundle_for_evaluation(
      parsed_schema.document, custom_resolver, dialect, schema_default_id,
      display_path, parsed_schema.positions)};

  const auto frame{frame_for_evaluation(bundled, custom_resolver, dialect,
                                        schema_default_id, display_path,
                                        parsed_schema.positions)};

  const auto schema_template{
      compile_for_evaluation(bundled, custom_resolver, frame, frame.root(),
                             sourcemeta::blaze::Mode::Exhaustive, std::nullopt,
                             display_path, parsed_schema.positions)};

  // Here is where what goes on the wire parts ways with what the response is
  // checked against. Spelling a schema for a provider loses constraints in both
  // directions, and checking the answer against what was sent would make every
  // constraint lost on the way out invisible, which is the very thing this
  // command exists to find
  auto wire{bundled};

  // A schema that declares no dialect was read under the one the caller named,
  // and what goes out says so rather than leaving whoever receives it to guess.
  // This happens before the conversion, so that what is written here is lifted
  // along with everything else rather than standing apart from it
  if (!wire.defines("$schema") && !dialect.empty()) {
    wire.assign("$schema", sourcemeta::core::JSON{dialect});
  }

  upgrade_for_wire(wire, custom_resolver, upgrade_target, dialect,
                   schema_default_id, display_path, parsed_schema.positions);

  if (options.contains("without-id")) {
    sourcemeta::blaze::for_editor(wire, sourcemeta::core::schema_walker,
                                  custom_resolver, dialect);
  }

  format_schema(wire, custom_resolver, dialect);

  // Nothing reaches a provider without saying which dialect it is written
  // against. A schema that declares none is read under a named dialect or not
  // read at all, so by here there is always one to state
  assert(wire.is_object());
  assert(wire.defines("$schema"));

  auto body{make_request_body(wire, options.at("model").front(),
                              options.at("ask").front())};
  if (options.contains("param")) {
    for (const auto &parameter : options.at("param")) {
      overlay_parameter(body, parameter);
    }
  }

  const auto url{options.at("url").front()};

  if (options.contains("dry-run")) {
    print_dry_run(options, url, body);
    return;
  }

  sourcemeta::core::HTTPSystemRequest request{
      std::string{url}, sourcemeta::core::HTTPMethod::POST};
  // A redirect would carry the credential to wherever it points, and the one
  // place this request is meant for is the one that was named
  request.follow_redirects(false);
  request.maximum_response_size(MAXIMUM_RESPONSE_SIZE);
  request.timeout(timeout);

  for (const auto &header : collect_http_headers(options)) {
    // Wiping storage, as a header is how the credential for the completion
    // request arrives
    request.header(std::string{header.first},
                   sourcemeta::core::SecureString{header.second});
  }

  std::ostringstream serialized;
  sourcemeta::core::stringify(body, serialized);
  request.body(serialized.str(), "application/json");

  LOG_VERBOSE(options) << "Sending a completion request: " << url << "\n";

  // Sent once and never retried. A completion is not idempotent and it costs
  // money, so a request that fails is reported rather than repeated
  const auto response{request.send()};

  LOG_VERBOSE(options) << "Received HTTP " << response.status.code << "\n";

  if (!is_successful(response.status)) {
    report_response_body(options, "Unsuccessful HTTP response", url, response);
    throw Fail{exit_code_for_status(response.status)};
  }

  if (options.contains("raw")) {
    print_verbatim(response.body, std::cout);
    return;
  }

  std::optional<sourcemeta::core::JSON> envelope;
  try {
    envelope = sourcemeta::core::parse_json(response.body);
  } catch (const sourcemeta::core::JSONParseError &) {
    report_response_body(options, "The response body is not valid JSON", url,
                         response);
    throw Fail{EXIT_NOT_SUPPORTED};
  }

  // An envelope that came back successfully and that we cannot read is a gap in
  // what this command knows rather than a mistake the user made, so it gets the
  // same passthrough an error body does
  const auto *content{generated_content(envelope.value())};
  if (content == nullptr) {
    report_response_body(options,
                         "The response does not carry a generated document",
                         url, response);
    throw Fail{EXIT_NOT_SUPPORTED};
  }

  const auto &generated{content->to_string()};

  sourcemeta::core::PointerPositionTracker positions;
  auto property_storage{std::make_shared<std::deque<std::string>>()};
  sourcemeta::core::JSON document{sourcemeta::core::JSON{nullptr}};
  std::istringstream generated_stream{generated};

  const auto json_output{options.contains("json")};
  const auto trace{options.contains("trace")};

  // Coordinates are computed against the string the model emitted, so they
  // point at what it literally wrote
  try {
    auto callback{make_position_callback(positions, property_storage)};
    sourcemeta::core::parse_json(generated_stream, document, callback);
  } catch (const sourcemeta::core::JSONParseError &error) {
    print_verbatim(generated, std::cout);
    if (json_output) {
      auto result{sourcemeta::core::JSON::make_object()};
      result.assign("valid", sourcemeta::core::JSON{false});
      result.assign("error",
                    sourcemeta::core::JSON{"The generated document is not "
                                           "valid JSON"});
      result.assign("line", sourcemeta::core::JSON{
                                static_cast<std::size_t>(error.line())});
      result.assign("column", sourcemeta::core::JSON{
                                  static_cast<std::size_t>(error.column())});
      sourcemeta::core::prettify(result, std::cout);
      std::cout << "\n";
    } else {
      std::cerr << "\n"
                << format_validation_status(ValidationStatus::Fail) << " "
                << url << "\n";
      std::cerr << "error: The generated document is not valid JSON\n";
      std::cerr << "  " << error.what() << "\n";
      std::cerr << "    at line " << error.line() << ", column "
                << error.column() << "\n";
    }

    throw Fail{EXIT_EXPECTED_FAILURE};
  }

  // Bytes past the first document mean the model wrote more than the one answer
  // the schema asked for, which no amount of validation would notice
  if (!at_end_of_stream(generated, generated_stream)) {
    print_verbatim(generated, std::cout);
    if (json_output) {
      auto result{sourcemeta::core::JSON::make_object()};
      result.assign("valid", sourcemeta::core::JSON{false});
      result.assign("error", sourcemeta::core::JSON{
                                 "The generated document is followed by more "
                                 "than one document"});
      sourcemeta::core::prettify(result, std::cout);
      std::cout << "\n";
    } else {
      std::cerr << "\n"
                << format_validation_status(ValidationStatus::Fail) << " "
                << url << "\n";
      std::cerr << "error: The generated document is followed by more than one "
                   "document\n";
    }

    throw Fail{EXIT_EXPECTED_FAILURE};
  }

  LOG_VERBOSE(options)
      << "Validating the generated document against the schema\n";

  sourcemeta::blaze::Evaluator evaluator;
  bool result{true};

  if (json_output) {
    const auto standard{sourcemeta::blaze::standard(
        evaluator, schema_template, document,
        sourcemeta::blaze::StandardOutput::Basic, positions)};
    assert(standard.is_object());
    assert(standard.defines("valid"));
    assert(standard.at("valid").is_boolean());
    result = standard.at("valid").to_boolean();

    auto report{sourcemeta::core::JSON::make_object()};
    report.assign("valid", sourcemeta::core::JSON{result});
    report.assign("raw", sourcemeta::core::JSON{generated});
    report.assign("document", document);
    report.assign("output", standard);

    const auto *reason{finish_reason(envelope.value())};
    if (reason != nullptr) {
      report.assign("finishReason", *reason);
    }

    const auto usage{usage_json(envelope.value())};
    if (usage.has_value()) {
      report.assign("usage", usage.value());
    }

    sourcemeta::core::prettify(report, std::cout);
    std::cout << "\n";
  } else if (trace) {
    print_verbatim(generated, std::cout);
    sourcemeta::blaze::TraceOutput trace_output{
        schema_template, trace_callback(positions, std::cout)};
    result =
        evaluator.validate(schema_template, document, std::ref(trace_output));
  } else {
    print_verbatim(generated, std::cout);
    sourcemeta::blaze::SimpleOutput output{document};
    result = evaluator.validate(schema_template, document, std::ref(output));
    if (!result) {
      std::cerr << "\n"
                << format_validation_status(ValidationStatus::Fail) << " "
                << url << "\n";
      print(output, positions, std::cerr, "error:", {},
            "The generated document does not conform to the schema");
    }
  }

  // The machine-readable report already carries these, so this is only for
  // whoever is reading along
  if (!json_output) {
    report_usage(envelope.value());
  }

  if (result) {
    LOG_VERBOSE(options) << format_validation_status(ValidationStatus::Pass)
                         << " " << url << "\n";
    return;
  }

  throw Fail{EXIT_EXPECTED_FAILURE};
}
