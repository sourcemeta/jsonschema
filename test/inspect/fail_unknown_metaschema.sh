#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://example.com/unknown",
  "$id": "https://example.com",
  "$ref": "nested"
}
EOF

"$1" inspect "$TMP/schema.json" 2>"$TMP/stderr.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Could not resolve the requested schema
  at https://example.com/unknown
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
