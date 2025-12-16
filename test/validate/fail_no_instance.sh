#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema",
  "type": "string"
}
EOF

"$1" validate "$TMP/schema.json" 2>"$TMP/stderr.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
error: In addition to the schema, you must also pass an argument
that represents the instance to validate against

For example: jsonschema validate path/to/schema.json path/to/instance.json
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" validate "$TMP/schema.json" --json > "$TMP/stdout.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
{
  "error": "In addition to the schema, you must also pass an argument\nthat represents the instance to validate against"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
