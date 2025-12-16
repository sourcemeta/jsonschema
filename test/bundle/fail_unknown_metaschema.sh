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
  "$id": "https://example.com",
  "$ref": "nested"
}
EOF

"$1" bundle "$TMP/schema.json" 2>"$TMP/stderr.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Could not resolve the metaschema of the schema
  at identifier https://example.com/unknown
  at file path $(realpath "$TMP")/schema.json

This is likely because you forgot to import such schema using \`--resolve/-r\`
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" bundle "$TMP/schema.json" --json >"$TMP/stdout.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "Could not resolve the metaschema of the schema",
  "identifier": "https://example.com/unknown",
  "filePath": "$(realpath "$TMP")/schema.json"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
