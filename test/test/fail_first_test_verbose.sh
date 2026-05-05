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
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/test.json"
{
  "target": "https://example.com",
  "tests": [
    {
      "description": "First test fails",
      "valid": true,
      "data": 1
    },
    {
      "description": "Second test passes",
      "valid": true,
      "data": "foo"
    }
  ]
}
EOF

"$1" test "$TMP/test.json" --resolve "$TMP/schema.json" --verbose 1> "$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Test assertion failure
test "$EXIT_CODE" = "2"

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP")/test.json:
  1/2 FAIL First test fails

error: Schema validation failure
  The value was expected to be of type string but it was of type integer
    at instance location ""
    at evaluate path "/type"

  2/2 PASS Second test passes
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
