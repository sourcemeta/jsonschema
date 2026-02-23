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
  "allOf": [ { "$ref": "https://schemas.sourcemeta.com/self/v1/api/schemas/stats/jsonschema/2020-12/schema" } ]
}
EOF

"$1" bundle "$TMP/schema.json" --http 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4" || exit 1

cat << EOF > "$TMP/expected.txt"
error: The JSON document is not a valid JSON Schema
  at identifier https://schemas.sourcemeta.com/self/v1/api/schemas/stats/jsonschema/2020-12/schema
  at file path $(realpath "$TMP")/schema.json
  at location "/allOf/0/\$ref"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
