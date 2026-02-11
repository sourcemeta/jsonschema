#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cd "$TMP"

"$1" install a b c > "$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
error: The install command takes either zero or two positional arguments

For example: jsonschema install https://example.com/schema ./vendor/schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" install --json a b c > "$TMP/output_json.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected_json.txt"
{
  "error": "The install command takes either zero or two positional arguments"
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"
