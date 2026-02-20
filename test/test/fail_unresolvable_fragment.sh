#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
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

cat << 'EOF' > "$TMP/test.json"
{
  "target": "https://example.com#/foo",
  "tests": [
    {
      "valid": true,
      "data": {}
    },
    {
      "valid": true,
      "data": { "type": 1 }
    }
  ]
}
EOF

"$1" test "$TMP/test.json" --resolve "$TMP/schema.json" --verbose 1> "$TMP/output.txt" 2>&1 \
  && CODE="$?" || CODE="$?"
test "$CODE" = "3" || exit 1

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP")/test.json:
error: Could not resolve schema under test
  at identifier https://example.com#/foo
  at file path $(realpath "$TMP")/test.json

This is likely because you forgot to import such schema using \`--resolve/-r\`
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
