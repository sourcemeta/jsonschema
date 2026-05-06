#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
[ 1, 2, 3 ]
EOF

"$1" upgrade "$TMP/schema.json" 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected.txt"
error: The schema file you provided does not represent a valid JSON Schema
  at file path $(realpath "$TMP")/schema.json
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" upgrade "$TMP/schema.json" --json > "$TMP/stdout.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected_json.txt"
{
  "error": "The schema file you provided does not represent a valid JSON Schema",
  "filePath": "$(realpath "$TMP")/schema.json"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected_json.txt"
