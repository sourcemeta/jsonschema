#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "type" string
}
EOF

"$1" compile "$TMP/schema.json" 2>"$TMP/stderr.txt" \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Failed to parse the JSON document
  at line 2
  at column 10
  at file path $(realpath "$TMP")/schema.json
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" compile "$TMP/schema.json" --json >"$TMP/stdout.txt" \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "Failed to parse the JSON document",
  "line": 2,
  "column": 10,
  "filePath": "$(realpath "$TMP")/schema.json"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
