#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cd "$TMP"

EXIT_CODE=0
"$1" install --header "no-colon-here" > "$TMP/output.txt" 2>&1 || EXIT_CODE=$?

cat << 'EOF' > "$TMP/expected.txt"
error: HTTP headers must be in the form `Name: Value`

For example: jsonschema install --header "Authorization: Bearer ${TOKEN}"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

# Invalid CLI arguments
test "$EXIT_CODE" = "5"

EXIT_CODE_JSON=0
"$1" install --json --header "no-colon-here" \
  > "$TMP/output_json.txt" 2>&1 || EXIT_CODE_JSON=$?

cat << 'EOF' > "$TMP/expected_json.txt"
{
  "error": "HTTP headers must be in the form `Name: Value`"
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"

# Invalid CLI arguments
test "$EXIT_CODE_JSON" = "5"
