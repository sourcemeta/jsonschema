#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

CWD="$(pwd -P)"

echo '[ 1, 2, 3 ]' | "$1" canonicalize - 2>"$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4" || exit 1

cat << EOF > "$TMP/expected.txt"
error: The schema file you provided does not represent a valid JSON Schema
  at file path $CWD
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
echo '[ 1, 2, 3 ]' | "$1" canonicalize - --json >"$TMP/stdout.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "4" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "The schema file you provided does not represent a valid JSON Schema",
  "filePath": "$CWD"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
