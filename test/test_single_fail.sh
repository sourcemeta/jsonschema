#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
# shellcheck disable=SC2317
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
      "description": "Invalid type, expected to fail",
      "valid": true,
      "data": { "type": 1 }
    }
  ]
}
EOF

"$1" test "$TMP/test.json" && CODE="$?" || CODE="$?"

if [ "$CODE" = "0" ]
then
  echo "FAIL" 1>&2
  exit 1
else
  echo "PASS" 1>&2
  exit 0
fi
