#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir "$TMP/schemas"

cat << 'EOF' > "$TMP/schemas/schema.json"
{
  "id": "https://example.com",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema",
  "definitions": {
    "foo": { "type": "string" },
    "bar": { "type": "integer" }
  }
}
EOF

cat << 'EOF' > "$TMP/schemas/test.json"
{
  "target": "https://example.com#/definitions/foo",
  "tests": [
    {
      "description": "Fail",
      "valid": true,
      "data": 5
    }
  ]
}
EOF

"$1" test "$TMP/schemas/test.json" --verbose --resolve "$TMP" 1> "$TMP/output.txt" 2>&1 \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Could not determine the base dialect of the schema
  at file path $(realpath "$TMP")/schemas/test.json

Are you sure the input is a valid JSON Schema and its base dialect is known?
If the input does not declare the \`\$schema\` keyword, you might want to
explicitly declare a default dialect using \`--default-dialect/-d\`
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
