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
  "type": "object",
  "additionalProperties": false,
  "properties": {
    "foo": true,
    "bar": true
  }
}
EOF

cat << 'EOF' > "$TMP/test.json"
{
  "target": "https://example.com",
  "tests": [
    {
      "valid": false,
      "data": { "fo": true }
    }
  ]
}
EOF

"$1" test "$TMP/test.json" --resolve "$TMP/schema.json" 1> "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP")/test.json: PASS 1/1
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
