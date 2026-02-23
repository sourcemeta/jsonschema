#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/rule.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "require_type",
  "description": "test",
  "$ref": "https://example.com/nonexistent"
}
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

"$1" lint --rule "$TMP/rule.json" "$TMP/schema.json" \
  > "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Could not resolve the reference to an external schema
  at identifier https://example.com/nonexistent
  at file path $(realpath "$TMP")/rule.json

This is likely because you forgot to import such schema using \`--resolve/-r\`
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" lint --rule "$TMP/rule.json" --json "$TMP/schema.json" \
  > "$TMP/output_json.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4" || exit 1

cat << EOF > "$TMP/expected_json.txt"
{
  "error": "Could not resolve the reference to an external schema",
  "identifier": "https://example.com/nonexistent",
  "filePath": "$(realpath "$TMP")/rule.json"
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"
