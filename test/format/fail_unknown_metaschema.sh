#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://example.com/unknown",
  "title": "Test",
  "description": "Test schema",
  "properties": {
    "foo": {
      "type": "string"
    }
  }
}
EOF

"$1" fmt "$TMP/schema.json" >"$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Could not resolve the metaschema of the schema
  at identifier https://example.com/unknown
  at file path $(realpath "$TMP")/schema.json

This is likely because you forgot to import such schema using \`--resolve/-r\`
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

# JSON error
"$1" fmt "$TMP/schema.json" --json >"$TMP/output_json.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4" || exit 1

cat << EOF > "$TMP/expected_json.txt"
{
  "error": "Could not resolve the metaschema of the schema",
  "identifier": "https://example.com/unknown",
  "filePath": "$(realpath "$TMP")/schema.json"
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"
