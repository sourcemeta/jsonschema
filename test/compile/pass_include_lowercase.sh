#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$id": "https://example.com",
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

"$1" compile "$TMP/schema.json" --include test_schema > "$TMP/output.h"

cat << 'EOF' > "$TMP/expected.h"
#ifndef SOURCEMETA_JSONSCHEMA_INCLUDE_TEST_SCHEMA_H_
#define SOURCEMETA_JSONSCHEMA_INCLUDE_TEST_SCHEMA_H_

#ifdef __cplusplus
#include <string_view>
#endif

static const char TEST_SCHEMA_DATA[] =
  "\x5b\x66\x61\x6c\x73\x65\x2c\x74\x72\x75\x65\x2c\x5b\x22\x22\x2c"
  "\x22\x68\x74\x74\x70\x73\x3a\x2f\x2f\x65\x78\x61\x6d\x70\x6c\x65"
  "\x2e\x63\x6f\x6d\x22\x5d\x2c\x5b\x5b\x31\x31\x2c\x22\x2f\x74\x79"
  "\x70\x65\x22\x2c\x22\x22\x2c\x22\x23\x2f\x74\x79\x70\x65\x22\x2c"
  "\x32\x2c\x5b\x38\x2c\x34\x5d\x5d\x5d\x5d";
static const unsigned int TEST_SCHEMA_LENGTH = 74;

#ifdef __cplusplus
static constexpr std::string_view TEST_SCHEMA{TEST_SCHEMA_DATA, TEST_SCHEMA_LENGTH};
#endif

#endif
EOF

diff "$TMP/output.h" "$TMP/expected.h"
