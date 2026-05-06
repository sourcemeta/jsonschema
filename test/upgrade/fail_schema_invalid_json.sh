#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{ not json
EOF

"$1" upgrade "$TMP/schema.json" 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6"

cat << EOF > "$TMP/expected.txt"
error: Failed to parse the JSON document
  at line 1
  at column 3
  at file path $(realpath "$TMP")/schema.json
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" upgrade "$TMP/schema.json" --json > "$TMP/stdout.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6"

cat << EOF > "$TMP/expected_json.txt"
{
  "error": "Failed to parse the JSON document",
  "line": 1,
  "column": 3,
  "filePath": "$(realpath "$TMP")/schema.json"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected_json.txt"
