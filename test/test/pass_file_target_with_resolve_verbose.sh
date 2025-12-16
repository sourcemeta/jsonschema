#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/test.json"
{
  "target": "./schema.json",
  "$comment": "A random comment",
  "tests": [
    {
      "valid": true,
      "data": "foo"
    },
    {
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
Looking for target: file://$(realpath "$TMP")/schema.json
$(realpath "$TMP")/test.json:
  1/2 PASS <no description>
  2/2 PASS <no description>
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
