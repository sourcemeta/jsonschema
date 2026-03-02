#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

printf '   \n' | "$1" validate "$TMP/schema.json" - 2>"$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "6" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
error: Failed to parse the JSON document
  at line 2
  at column 1
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
printf '   \n' | "$1" validate "$TMP/schema.json" - --json >"$TMP/stdout.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "6" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
{
  "error": "Failed to parse the JSON document",
  "line": 2,
  "column": 1
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
