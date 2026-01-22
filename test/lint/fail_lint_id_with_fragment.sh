#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "#fragment"
}
EOF

"$1" lint "$TMP/schema.json" >"$TMP/output.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Identifiers must not contain non-empty fragments
  at identifier #fragment
  at file path $(realpath "$TMP")/schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" lint "$TMP/schema.json" --json >"$TMP/output.json" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.json"
{
  "error": "Identifiers must not contain non-empty fragments",
  "identifier": "#fragment",
  "filePath": "$(realpath "$TMP")/schema.json"
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
