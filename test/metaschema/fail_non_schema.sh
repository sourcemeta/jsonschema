#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/document.json"
[ { "foo": 1 } ]
EOF

"$1" metaschema "$TMP/document.json" 2>"$TMP/stderr.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: The schema file you provided does not represent a valid JSON Schema
  at file path $(realpath "$TMP/document.json")
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" metaschema "$TMP/document.json" --json >"$TMP/stdout.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "The schema file you provided does not represent a valid JSON Schema",
  "filePath": "$(realpath "$TMP/document.json")"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
