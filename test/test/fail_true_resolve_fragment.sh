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
      "description": "Fail",
      "valid": true,
      "data": 5
    }
  ]
}
EOF

"$1" test "$TMP/test.json" --resolve "$TMP/schema.json" 1> "$TMP/output.txt" 2>&1 \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP")/test.json:
  1/1 FAIL Fail

error: Schema validation failure
  The value was expected to be of type string but it was of type integer
    at instance location ""
    at evaluate path "/type"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
