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

cat << 'EOF' > "$TMP/instance.json"
{ "type": "string" }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" --http 2> "$TMP/stderr.txt" \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Failed to parse the JSON document at line 1 and column 1
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
