#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/document.json"
{ "type": "string" }
EOF

"$1" metaschema "$TMP/document.json" 2>"$TMP/stderr.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: The schema file does not declare a dialect
  $(realpath "$TMP/document.json")
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
