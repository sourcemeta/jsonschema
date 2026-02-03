#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "title": "Test",
  "type": "string",
  "$schema": "http://json-schema.org/draft-06/schema#",
  "$id": "https://example.com"
}
EOF

cd "$TMP"
"$1" lint "$TMP/schema.json" --fix --format >"$TMP/output.txt" 2>&1 \
  && CODE="$?" || CODE="$?"
test "$CODE" = "2" || exit 1

cat << 'EOF' > "$TMP/expected_output.txt"
schema.json:1:1:
  Set a non-empty description at the top level of the schema to explain what the definition is about in detail (top_level_description)
    at location ""
schema.json:1:1:
  Set a non-empty examples array at the top level of the schema to illustrate the expected data (top_level_examples)
    at location ""
EOF

diff "$TMP/output.txt" "$TMP/expected_output.txt"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "http://json-schema.org/draft-06/schema#",
  "$id": "https://example.com",
  "title": "Test",
  "type": "string"
}
EOF

diff "$TMP/schema.json" "$TMP/expected.json"

# JSON output
"$1" lint "$TMP/schema.json" --fix --format --json >"$TMP/output_json.txt" 2>&1 \
  && CODE="$?" || CODE="$?"
test "$CODE" = "2" || exit 1

cat << EOF > "$TMP/expected_output_json.txt"
{
  "valid": false,
  "health": 0,
  "errors": [
    {
      "path": "$(realpath "$TMP")/schema.json",
      "id": "top_level_description",
      "message": "Set a non-empty description at the top level of the schema to explain what the definition is about in detail",
      "description": null,
      "schemaLocation": "",
      "position": [ 1, 1, 6, 1 ]
    },
    {
      "path": "$(realpath "$TMP")/schema.json",
      "id": "top_level_examples",
      "message": "Set a non-empty examples array at the top level of the schema to illustrate the expected data",
      "description": null,
      "schemaLocation": "",
      "position": [ 1, 1, 6, 1 ]
    }
  ]
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_output_json.txt"
