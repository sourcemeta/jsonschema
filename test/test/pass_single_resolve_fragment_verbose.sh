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
  "definitions": {
    "foo": { "type": "string" },
    "bar": { "type": "integer" }
  }
}
EOF

cat << 'EOF' > "$TMP/test.json"
{
  "target": "https://example.com#/definitions/foo",
  "tests": [
    {
      "description": "First test",
      "valid": true,
      "data": "foo"
    },
    {
      "description": "Invalid type",
      "valid": false,
      "data": 1
    }
  ]
}
EOF

"$1" test "$TMP/test.json" --resolve "$TMP/schema.json" --verbose 1> "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Detecting schema resources from file: $(realpath "$TMP")/schema.json
Importing schema into the resolution context: file://$(realpath "$TMP")/schema.json
Importing schema into the resolution context: https://example.com
Looking for target: https://example.com#/definitions/foo
$(realpath "$TMP")/test.json:
  1/2 PASS First test
  2/2 PASS Invalid type
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
