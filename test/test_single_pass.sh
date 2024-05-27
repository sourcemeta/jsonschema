#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/test.json"
{
  "description": "My sample suite",
  "schema": "http://json-schema.org/draft-04/schema#",
  "tests": [
    {
      "description": "First test",
      "valid": true,
      "data": {}
    },
    {
      "description": "Invalid type",
      "valid": false,
      "data": { "type": 1 }
    }
  ]
}
EOF

"$1" test "$TMP/test.json"
