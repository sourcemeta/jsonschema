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
  "allOf": [
    {
      "anyOf": [
        { "required": [ "foo" ] },
        { "required": [ "bar" ] }
      ]
    },
    {
      "type": "integer"
    }
  ]
}
EOF

cat << 'EOF' > "$TMP/test.json"
{
  "target": "https://example.com",
  "tests": [
    {
      "valid": true,
      "data": { "bar": 1 }
    }
  ]
}
EOF

"$1" test "$TMP/test.json" --resolve "$TMP/schema.json" 1> "$TMP/output.txt" 2>&1 \
  && CODE="$?" || CODE="$?"
test "$CODE" = "2" || exit 1

cat "$TMP/output.txt"

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP")/test.json:
  1/1 FAIL <no description>

error: Schema validation failure
  The value was expected to be of type integer but it was of type object
    at instance location ""
    at evaluate path "/allOf/1/type"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
