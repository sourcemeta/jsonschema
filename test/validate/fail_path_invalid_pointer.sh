#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object"
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{}
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --path 'invalid~pointer' > "$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Invalid CLI arguments
test "$EXIT_CODE" = "5"

cat << EOF > "$TMP/expected.txt"
error: The JSON Pointer is not valid

For example: jsonschema validate path/to/schema.json path/to/instance.json --path '/components/schemas/User'
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

# JSON error
"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --path 'invalid~pointer' --json > "$TMP/stdout.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "5"

cat << EOF > "$TMP/expected.txt"
{
  "error": "The JSON Pointer is not valid"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
