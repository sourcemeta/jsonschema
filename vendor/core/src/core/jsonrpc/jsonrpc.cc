#include <sourcemeta/core/jsonrpc.h>

#include <sourcemeta/core/json.h>

#include <cstdint>  // std::int64_t
#include <optional> // std::optional, std::nullopt
#include <utility>  // std::move

namespace {

const auto JSONRPC_HASH_ID{sourcemeta::core::JSON::Object::hash("id")};
const auto JSONRPC_HASH_JSONRPC{
    sourcemeta::core::JSON::Object::hash("jsonrpc")};
const auto JSONRPC_HASH_METHOD{sourcemeta::core::JSON::Object::hash("method")};
const auto JSONRPC_HASH_RESULT{sourcemeta::core::JSON::Object::hash("result")};
const auto JSONRPC_HASH_ERROR{sourcemeta::core::JSON::Object::hash("error")};
const auto JSONRPC_HASH_CODE{sourcemeta::core::JSON::Object::hash("code")};
const auto JSONRPC_HASH_MESSAGE{
    sourcemeta::core::JSON::Object::hash("message")};
const auto JSONRPC_HASH_DATA{sourcemeta::core::JSON::Object::hash("data")};
const auto JSONRPC_HASH_PARAMS{sourcemeta::core::JSON::Object::hash("params")};

} // namespace

namespace sourcemeta::core {

auto jsonrpc_is_server_error(const std::int64_t code) -> bool {
  return code >= JSONRPC_CODE_SERVER_ERROR_MIN &&
         code <= JSONRPC_CODE_SERVER_ERROR_MAX;
}

auto jsonrpc_is_batch(const sourcemeta::core::JSON &payload) -> bool {
  return payload.is_array();
}

auto jsonrpc_is_valid_batch(const sourcemeta::core::JSON &payload) -> bool {
  return jsonrpc_is_batch(payload) && !payload.empty();
}

auto jsonrpc_request_id(const sourcemeta::core::JSON &request)
    -> const sourcemeta::core::JSON * {
  if (!request.is_object()) {
    return nullptr;
  }
  const auto *identifier{request.try_at("id", JSONRPC_HASH_ID)};
  if (identifier == nullptr) {
    return nullptr;
  }
  if (!identifier->is_string() && !identifier->is_number() &&
      !identifier->is_null()) {
    return nullptr;
  }
  return identifier;
}

auto jsonrpc_is_request(const sourcemeta::core::JSON &request) -> bool {
  if (!request.is_object()) {
    return false;
  }
  const auto *jsonrpc_field{request.try_at("jsonrpc", JSONRPC_HASH_JSONRPC)};
  if (jsonrpc_field == nullptr || !jsonrpc_field->is_string() ||
      jsonrpc_field->to_string() != "2.0" ||
      jsonrpc_request_id(request) == nullptr) {
    return false;
  }
  const auto *parameters_field{request.try_at("params", JSONRPC_HASH_PARAMS)};
  if (parameters_field != nullptr && !parameters_field->is_object() &&
      !parameters_field->is_array()) {
    return false;
  }
  const auto *method_field{request.try_at("method", JSONRPC_HASH_METHOD)};
  return method_field != nullptr && method_field->is_string();
}

auto jsonrpc_method(const sourcemeta::core::JSON &request) -> JSON::StringView {
  if (!request.is_object()) {
    return {};
  }
  const auto *method_field{request.try_at("method", JSONRPC_HASH_METHOD)};
  if (method_field == nullptr || !method_field->is_string()) {
    return {};
  }
  return method_field->to_string();
}

auto jsonrpc_params(const sourcemeta::core::JSON &request)
    -> const sourcemeta::core::JSON * {
  if (!request.is_object()) {
    return nullptr;
  }
  const auto *parameters{request.try_at("params", JSONRPC_HASH_PARAMS)};
  if (parameters == nullptr ||
      (!parameters->is_object() && !parameters->is_array())) {
    return nullptr;
  }
  return parameters;
}

auto jsonrpc_is_notification(const sourcemeta::core::JSON &request) -> bool {
  if (!request.is_object()) {
    return false;
  }
  const auto *jsonrpc_field{request.try_at("jsonrpc", JSONRPC_HASH_JSONRPC)};
  if (jsonrpc_field == nullptr || !jsonrpc_field->is_string() ||
      jsonrpc_field->to_string() != "2.0" ||
      request.try_at("id", JSONRPC_HASH_ID) != nullptr) {
    return false;
  }
  const auto *parameters_field{request.try_at("params", JSONRPC_HASH_PARAMS)};
  if (parameters_field != nullptr && !parameters_field->is_object() &&
      !parameters_field->is_array()) {
    return false;
  }
  const auto *method_field{request.try_at("method", JSONRPC_HASH_METHOD)};
  return method_field != nullptr && method_field->is_string();
}

auto jsonrpc_make_success(const sourcemeta::core::JSON &identifier,
                          sourcemeta::core::JSON result)
    -> sourcemeta::core::JSON {
  auto envelope{sourcemeta::core::JSON::make_object()};
  envelope.assign_assume_new("jsonrpc", sourcemeta::core::JSON{"2.0"},
                             JSONRPC_HASH_JSONRPC);
  envelope.assign_assume_new("id", sourcemeta::core::JSON{identifier},
                             JSONRPC_HASH_ID);
  envelope.assign_assume_new("result", std::move(result), JSONRPC_HASH_RESULT);
  return envelope;
}

auto jsonrpc_make_success_empty(const sourcemeta::core::JSON &identifier)
    -> sourcemeta::core::JSON {
  return jsonrpc_make_success(identifier,
                              sourcemeta::core::JSON::make_object());
}

auto jsonrpc_make_error(const sourcemeta::core::JSON *identifier,
                        const std::int64_t code, const JSON::StringView message,
                        std::optional<sourcemeta::core::JSON> data)
    -> sourcemeta::core::JSON {
  auto envelope{sourcemeta::core::JSON::make_object()};
  envelope.assign_assume_new("jsonrpc", sourcemeta::core::JSON{"2.0"},
                             JSONRPC_HASH_JSONRPC);
  envelope.assign_assume_new("id",
                             identifier != nullptr
                                 ? sourcemeta::core::JSON{*identifier}
                                 : sourcemeta::core::JSON{nullptr},
                             JSONRPC_HASH_ID);
  auto error{sourcemeta::core::JSON::make_object()};
  error.assign_assume_new("code", sourcemeta::core::JSON{code},
                          JSONRPC_HASH_CODE);
  error.assign_assume_new("message", sourcemeta::core::JSON{message},
                          JSONRPC_HASH_MESSAGE);
  if (data.has_value()) {
    error.assign_assume_new("data", std::move(data.value()), JSONRPC_HASH_DATA);
  }
  envelope.assign_assume_new("error", std::move(error), JSONRPC_HASH_ERROR);
  return envelope;
}

auto jsonrpc_make_error_parse() -> const sourcemeta::core::JSON & {
  static const auto envelope{
      jsonrpc_make_error(nullptr, JSONRPC_CODE_PARSE, "Parse error")};
  return envelope;
}

auto jsonrpc_make_error_invalid_request(
    const sourcemeta::core::JSON *identifier) -> sourcemeta::core::JSON {
  return jsonrpc_make_error(identifier, JSONRPC_CODE_INVALID_REQUEST,
                            "Invalid Request");
}

auto jsonrpc_make_error_method_not_found(
    const sourcemeta::core::JSON &identifier) -> sourcemeta::core::JSON {
  return jsonrpc_make_error(&identifier, JSONRPC_CODE_METHOD_NOT_FOUND,
                            "Method not found");
}

auto jsonrpc_make_error_invalid_params(
    const sourcemeta::core::JSON &identifier,
    std::optional<sourcemeta::core::JSON> data) -> sourcemeta::core::JSON {
  return jsonrpc_make_error(&identifier, JSONRPC_CODE_INVALID_PARAMS,
                            "Invalid params", std::move(data));
}

auto jsonrpc_make_error_internal(const sourcemeta::core::JSON *identifier)
    -> sourcemeta::core::JSON {
  return jsonrpc_make_error(identifier, JSONRPC_CODE_INTERNAL,
                            "Internal error");
}

} // namespace sourcemeta::core
