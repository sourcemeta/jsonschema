#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/rule.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "enum_with_type",
  "description": "conflicting rule",
  "required": [ "type" ]
}
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

"$1" lint --rule "$TMP/rule.json" "$TMP/schema.json" \
  > "$TMP/output.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: A lint rule with this name already exists
  at file path $(realpath "$TMP")/rule.json
  at rule enum_with_type
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" lint --rule "$TMP/rule.json" --json "$TMP/schema.json" \
  > "$TMP/output_json.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected_json.txt"
{
  "error": "A lint rule with this name already exists",
  "filePath": "$(realpath "$TMP")/rule.json",
  "rule": "enum_with_type"
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"
