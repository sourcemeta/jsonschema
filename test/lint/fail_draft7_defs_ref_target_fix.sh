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
  "allOf": [
    { "$ref": "#/$defs/not-a-schema" }
  ],
  "$defs": {
    "not-a-schema": {
      "type": "string"
    }
  },
  "default": "test"
}
EOF

"$1" lint "$TMP/schema.json" --fix 2>"$TMP/stderr.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4" || exit 1

cat << EOF > "$TMP/expected.txt"
.
error: The referenced schema is not considered to be a valid subschema given the dialect and vocabularies in use
  at identifier file://$(realpath "$TMP")/schema.json#/x-\$defs/not-a-schema
  at file path $(realpath "$TMP")/schema.json
  at location "/x-\$defs"

Are you sure the reported location is a valid JSON Schema keyword in this dialect?
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" lint "$TMP/schema.json" --fix --json >"$TMP/stdout.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "The referenced schema is not considered to be a valid subschema given the dialect and vocabularies in use",
  "identifier": "file://$(realpath "$TMP")/schema.json#/x-\$defs/not-a-schema",
  "filePath": "$(realpath "$TMP")/schema.json",
  "location": "/x-\$defs"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
