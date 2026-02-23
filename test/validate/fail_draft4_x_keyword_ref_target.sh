#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "allOf": [
    {
      "$ref": "#/allOf/1/additionalProperties/x-this-is-invalid/$defs/test"
    },
    {
      "additionalProperties": {
        "x-this-is-invalid": {
          "$defs": {
            "test": {
              "type": "string"
            }
          }
        }
      }
    }
  ]
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{}
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" 2>"$TMP/stderr.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4" || exit 1

cat << EOF > "$TMP/expected.txt"
error: The referenced schema is not considered to be a valid subschema given the dialect and vocabularies in use
  at identifier file://$(realpath "$TMP")/schema.json#/allOf/1/additionalProperties/x-this-is-invalid/\$defs/test
  at file path $(realpath "$TMP")/schema.json
  at location "/allOf/1/additionalProperties/x-this-is-invalid"

Are you sure the reported location is a valid JSON Schema keyword in this dialect?
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" validate "$TMP/schema.json" "$TMP/instance.json" --json >"$TMP/stdout.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "The referenced schema is not considered to be a valid subschema given the dialect and vocabularies in use",
  "identifier": "file://$(realpath "$TMP")/schema.json#/allOf/1/additionalProperties/x-this-is-invalid/\$defs/test",
  "filePath": "$(realpath "$TMP")/schema.json",
  "location": "/allOf/1/additionalProperties/x-this-is-invalid"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
