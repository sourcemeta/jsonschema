#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "title": "Test",
  "description": "Test schema",
  "allOf": [ { "$ref": "https://one.sourcemeta.com" } ]
}
EOF

"$1" bundle "$TMP/schema.json" --http 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Failed to parse the JSON document
  at line 2
  at column 1
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
