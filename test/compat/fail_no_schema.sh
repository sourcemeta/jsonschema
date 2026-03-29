#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

"$1" compat 2> "$TMP/stderr.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Invalid CLI arguments
test "$EXIT_CODE" = "5"

cat << 'EOF' > "$TMP/expected.txt"
error: This command expects a path to a base schema and a path to a
candidate schema

For example: jsonschema compat path/to/base.schema.json path/to/candidate.schema.json
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

"$1" compat --json > "$TMP/stdout.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Invalid CLI arguments
test "$EXIT_CODE" = "5"

cat << 'EOF' > "$TMP/expected.txt"
{
  "error": "This command expects a path to a base schema and a path to a\ncandidate schema"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
