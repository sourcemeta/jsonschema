#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test schema",
  "$id": "https://example.com",
  "$ref": "nested"
}
EOF

cat << 'EOF' > "$TMP/invalid.json"
{ xxx }
EOF

"$1" bundle "$TMP/schema.json" \
  --resolve "$TMP/invalid.json" 2>"$TMP/stderr.txt" \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Failed to parse the JSON document
  at line 1
  at column 3
  at file path $(realpath "$TMP")/invalid.json
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" bundle "$TMP/schema.json" \
  --resolve "$TMP/invalid.json" --json >"$TMP/stdout.txt" \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "Failed to parse the JSON document",
  "line": 1,
  "column": 3,
  "filePath": "$(realpath "$TMP")/invalid.json"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
