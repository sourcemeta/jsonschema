#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema",
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
  ],
  "default": "test"
}
EOF

"$1" lint "$TMP/schema.json" --fix 2>"$TMP/stderr.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

# After --fix elevates the allOf wrapper, the path changes
cat << EOF > "$TMP/expected.txt"
error: The referenced schema is not considered to be a valid subschema given the dialect and vocabularies in use
  at identifier file://$(realpath "$TMP")/schema.json#/additionalProperties/x-this-is-invalid/\$defs/test
  at file path $(realpath "$TMP")/schema.json
  at location "/additionalProperties/x-this-is-invalid"

Are you sure the reported location is a valid JSON Schema keyword in this dialect?
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" lint "$TMP/schema.json" --fix --json >"$TMP/stdout.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "The referenced schema is not considered to be a valid subschema given the dialect and vocabularies in use",
  "identifier": "file://$(realpath "$TMP")/schema.json#/additionalProperties/x-this-is-invalid/\$defs/test",
  "filePath": "$(realpath "$TMP")/schema.json",
  "location": "/additionalProperties/x-this-is-invalid"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
