#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/test.json"
{
  "target": {
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "string"
  },
  "tests": [
    { "valid": true, "data": "hello" }
  ]
}
EOF

"$1" test "$TMP/test.json" --jobs 0 \
  > "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Invalid CLI arguments
test "$EXIT_CODE" = "5"

cat << 'EOF' > "$TMP/expected.txt"
error: The --jobs option must be a positive integer
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" test "$TMP/test.json" --jobs foo \
  > "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Invalid CLI arguments
test "$EXIT_CODE" = "5"

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" test "$TMP/test.json" --jobs 0 --json \
  > "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Invalid CLI arguments
test "$EXIT_CODE" = "5"

cat << 'EOF' > "$TMP/expected.txt"
{
  "error": "The --jobs option must be a positive integer"
}
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
