#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

echo '{ "foo" 1 }' | "$1" fmt - 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6"

cat << 'EOF' > "$TMP/expected.txt"
error: Failed to parse the JSON document
  at line 1
  at column 9
  at file path /dev/stdin
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
echo '{ "foo" 1 }' | "$1" fmt - --json > "$TMP/stdout.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6"

cat << 'EOF' > "$TMP/expected.txt"
{
  "error": "Failed to parse the JSON document",
  "line": 1,
  "column": 9,
  "filePath": "/dev/stdin"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
