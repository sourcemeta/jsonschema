#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir "$TMP/schemas"

cat << 'EOF' > "$TMP/schemas/one.json"
{
  "id": "https://example.com/one",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": [ "string", "number" ]
}
EOF

cat << 'EOF' > "$TMP/schemas/two.json"
{
  "id": "https://example.com/two",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": [ "string", "number" ]
}
EOF

cat << 'EOF' > "$TMP/test.json"
{
  "target": [
    "https://example.com/one",
    "https://example.com/two"
  ],
  "tests": [
    {
      "description": "String is valid",
      "valid": true,
      "data": "foo"
    },
    {
      "description": "Object is invalid",
      "valid": false,
      "data": {}
    }
  ]
}
EOF

"$1" test "$TMP/test.json" --resolve "$TMP/schemas" --verbose 1> "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP")/test.json:
  https://example.com/one:
    1/4 PASS String is valid
    2/4 PASS Object is invalid
  https://example.com/two:
    3/4 PASS String is valid
    4/4 PASS Object is invalid
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
