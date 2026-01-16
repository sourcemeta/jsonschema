#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string",
  "if": { "minLength": 5 },
  "then": { "maxLength": 10 }
}
EOF

"$1" codegen "$TMP/schema.json" --target typescript \
  2>"$TMP/stderr.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Unsupported keyword in subschema
  at file path $(realpath "$TMP")/schema.json
  at keyword if
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" codegen "$TMP/schema.json" --target typescript --json \
  >"$TMP/stdout.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "Unsupported keyword in subschema",
  "filePath": "$(realpath "$TMP")/schema.json",
  "keyword": "if"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
