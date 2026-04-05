#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/document.json"
{
  "components": {
    "schemas": {
      "Address": {
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "type": "object"
      },
      "User": {
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "$ref": "#/components/schemas/Address"
      }
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{}
EOF

"$1" validate "$TMP/document.json" "$TMP/instance.json" \
  --path "/components/schemas/User" 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error: $ref outside extracted subtree cannot be resolved during bundling
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected.txt"
error: Could not resolve schema reference
  at identifier file://$(realpath "$TMP")/document.json#/components/schemas/Address
  at file path $(realpath "$TMP")/document.json
  at location "/\$ref"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" validate "$TMP/document.json" "$TMP/instance.json" \
  --path "/components/schemas/User" --json > "$TMP/stdout.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected.txt"
{
  "error": "Could not resolve schema reference",
  "identifier": "file://$(realpath "$TMP")/document.json#/components/schemas/Address",
  "filePath": "$(realpath "$TMP")/document.json",
  "location": "/\$ref"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"