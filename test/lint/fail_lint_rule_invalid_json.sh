#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/rule.json"
not json
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

"$1" lint --rule "$TMP/rule.json" "$TMP/schema.json" \
  > "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Failed to parse the JSON document
  at line 1
  at column 2
  at file path $(realpath "$TMP")/rule.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" lint --rule "$TMP/rule.json" --json "$TMP/schema.json" \
  > "$TMP/output_json.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6" || exit 1

cat << EOF > "$TMP/expected_json.txt"
{
  "error": "Failed to parse the JSON document",
  "line": 1,
  "column": 2,
  "filePath": "$(realpath "$TMP")/rule.json"
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"
