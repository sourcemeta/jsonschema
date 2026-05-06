#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/somedir"

"$1" upgrade "$TMP/somedir" 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6"

cat << EOF > "$TMP/expected.txt"
error: The input was supposed to be a file but it is a directory
  at file path $(realpath "$TMP")/somedir
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" upgrade "$TMP/somedir" --json > "$TMP/stdout.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6"

cat << EOF > "$TMP/expected_json.txt"
{
  "error": "The input was supposed to be a file but it is a directory",
  "filePath": "$(realpath "$TMP")/somedir"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected_json.txt"
