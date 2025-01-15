#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "allOf": [ { "$ref": "https://example.com" } ]
}
EOF

"$1" bundle "$TMP/schema.json" --http --verbose 2> "$TMP/stderr.txt" \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
Resolving over HTTP: https://example.com
error: Failed to parse the JSON document at line 1 and column 1
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
