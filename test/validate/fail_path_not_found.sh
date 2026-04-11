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
      "User": {
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "type": "object"
      }
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{}
EOF

"$1" validate "$TMP/document.json" "$TMP/instance.json" \
  --path "/components/schemas/NonExistent" 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Invalid CLI arguments (path not found)
test "$EXIT_CODE" = "5"

cat << EOF > "$TMP/expected.txt"
error: The JSON Pointer does not resolve to a value in the document
  at file path $(realpath "$TMP")/document.json
  at path /components/schemas/NonExistent
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" validate "$TMP/document.json" "$TMP/instance.json" \
  --path "/components/schemas/NonExistent" --json > "$TMP/stdout.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "5"

cat << EOF > "$TMP/expected.txt"
{
  "error": "The JSON Pointer does not resolve to a value in the document",
  "filePath": "$(realpath "$TMP")/document.json",
  "pointer": "/components/schemas/NonExistent"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
