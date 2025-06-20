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
error: Could not determine the base dialect of the schema
  $(realpath "$TMP/document.json")

Are you sure the input is a valid JSON Schema and its base dialect is known?
If the input does not declare the \$schema keyword, you might want to
explicitly declare a default dialect using --default-dialect/-d
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
