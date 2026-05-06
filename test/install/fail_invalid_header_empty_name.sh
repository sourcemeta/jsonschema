#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cd "$TMP"

"$1" install --header ": value" \
  > "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Invalid CLI arguments
test "$EXIT_CODE" = "5"

cat << 'EOF' > "$TMP/expected.txt"
error: HTTP header names cannot be empty

For example: jsonschema install --header "Authorization: Bearer ${TOKEN}"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" install --json --header ": value" \
  > "$TMP/output_json.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Invalid CLI arguments
test "$EXIT_CODE" = "5"

cat << 'EOF' > "$TMP/expected_json.txt"
{
  "error": "HTTP header names cannot be empty"
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"
