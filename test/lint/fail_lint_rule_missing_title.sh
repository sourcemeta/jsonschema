#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/rule.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "description": "missing title",
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

printf '%s\n' \
  "error: The schema rule title is missing or not a string" \
  "  at identifier " \
  "  at file path $(realpath "$TMP")/rule.json" \
  > "$TMP/expected.txt"

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" lint --rule "$TMP/rule.json" --json "$TMP/schema.json" \
  > "$TMP/output_json.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected_json.txt"
{
  "error": "The schema rule title is missing or not a string",
  "identifier": "",
  "filePath": "$(realpath "$TMP")/rule.json"
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"
