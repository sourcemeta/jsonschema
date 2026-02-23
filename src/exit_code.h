#ifndef SOURCEMETA_JSONSCHEMA_CLI_EXIT_CODE_H_
#define SOURCEMETA_JSONSCHEMA_CLI_EXIT_CODE_H_

namespace sourcemeta::jsonschema {

constexpr int EXIT_UNEXPECTED_ERROR = 1;
constexpr int EXIT_EXPECTED_FAILURE = 2;
constexpr int EXIT_NOT_SUPPORTED = 3;
constexpr int EXIT_SCHEMA_INPUT_ERROR = 4;
constexpr int EXIT_INVALID_CLI_ARGUMENTS = 5;
constexpr int EXIT_OTHER_INPUT_ERROR = 6;

} // namespace sourcemeta::jsonschema

#endif
