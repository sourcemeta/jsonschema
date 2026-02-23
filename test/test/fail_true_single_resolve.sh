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
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/test.json"
{
  "target": "https://example.com",
  "tests": [
    {
      "description": "First test",
      "valid": true,
      "data": "foo"
    },
    {
      "description": "Unexpected",
      "valid": true,
      "data": 1
    },
    {
      "description": "Invalid type",
      "valid": false,
      "data": 1
    }
  ]
}
EOF

"$1" test "$TMP/test.json" --resolve "$TMP/schema.json" 1> "$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Test assertion failure
test "$EXIT_CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP")/test.json:
  2/3 FAIL Unexpected

error: Schema validation failure
  The value was expected to be of type string but it was of type integer
    at instance location ""
    at evaluate path "/type"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
