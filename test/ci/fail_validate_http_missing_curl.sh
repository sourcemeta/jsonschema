#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "title": "Test",
  "description": "Test schema",
  "allOf": [ { "$ref": "https://one.sourcemeta.com" } ]
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "type": "string" }
EOF

SOURCEMETA_CORE_CURL_SO="$TMP/does-not-exist.so" \
  "$1" validate "$TMP/schema.json" "$TMP/instance.json" --http \
  > "$TMP/stdout.txt" 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Not supported
test "$EXIT_CODE" = "3"

cat << EOF > "$TMP/expected.txt"
error: Could not load the cURL library from the configured path
  with environment variable SOURCEMETA_CORE_CURL_SO
  with paths
  - $TMP/does-not-exist.so
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

SOURCEMETA_CORE_CURL_SO="$TMP/does-not-exist.so" \
  "$1" validate "$TMP/schema.json" "$TMP/instance.json" --http --json \
  > "$TMP/stdout_json.txt" 2> "$TMP/stderr_json.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Not supported
test "$EXIT_CODE" = "3"

cat << EOF > "$TMP/expected_json.txt"
{
  "error": "Could not load the cURL library from the configured path",
  "environmentVariable": "SOURCEMETA_CORE_CURL_SO",
  "paths": [ "$TMP/does-not-exist.so" ]
}
EOF

diff "$TMP/stdout_json.txt" "$TMP/expected_json.txt"
