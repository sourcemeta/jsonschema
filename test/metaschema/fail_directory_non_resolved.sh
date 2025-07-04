#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir "$TMP/schemas"

cat << 'EOF' > "$TMP/schemas/schema_1.json"
{
  "$schema": "http://example.com",
  "type": "string"
}
EOF

"$1" metaschema "$TMP/schemas" > "$TMP/stderr.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Could not resolve the metaschema of the schema
  http://example.com
  at $(realpath "$TMP")/schemas/schema_1.json

This is likely because you forgot to import such schema using --resolve/-r
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
