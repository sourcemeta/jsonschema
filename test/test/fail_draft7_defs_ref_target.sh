#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$id": "https://example.com/draft7-defs-schema",
  "$schema": "http://json-schema.org/draft-07/schema#",
  "allOf": [
    { "$ref": "#/$defs/not-a-schema" }
  ],
  "$defs": {
    "not-a-schema": {
      "type": "string"
    }
  }
}
EOF

cat << 'EOF' > "$TMP/test.json"
{
  "target": "https://example.com/draft7-defs-schema",
  "tests": [
    {
      "description": "Test case",
      "valid": true,
      "data": "hello"
    }
  ]
}
EOF

"$1" test "$TMP/test.json" --resolve "$TMP/schema.json" 2>"$TMP/stderr.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4" || exit 1

cat << EOF > "$TMP/expected.txt"
error: The referenced schema is not considered to be a valid subschema given the dialect and vocabularies in use
  at identifier https://example.com/draft7-defs-schema#/\$defs/not-a-schema
  at file path $(realpath "$TMP")/test.json
  at location "/\$defs/https:~1~1example.com~1draft7-defs-schema/\$defs"

Maybe you meant to use \`definitions\` instead of \`\$defs\` in this dialect?
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" test "$TMP/test.json" --resolve "$TMP/schema.json" --json >"$TMP/stdout.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "The referenced schema is not considered to be a valid subschema given the dialect and vocabularies in use",
  "identifier": "https://example.com/draft7-defs-schema#/\$defs/not-a-schema",
  "filePath": "$(realpath "$TMP")/test.json",
  "location": "/\$defs/https:~1~1example.com~1draft7-defs-schema/\$defs"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
