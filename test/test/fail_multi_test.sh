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

mkdir "$TMP/tests"

cat << 'EOF' > "$TMP/tests/1.json"
{
  "target": "https://example.com",
  "tests": [
    {
      "description": "First test",
      "valid": true,
      "data": "foo"
    },
    {
      "description": "First failure",
      "valid": true,
      "data": 1
    }
  ]
}
EOF

cat << 'EOF' > "$TMP/tests/2.json"
{
  "target": "https://example.com",
  "tests": [
    {
      "description": "Invalid type",
      "valid": false,
      "data": 1
    }
  ]
}
EOF

cat << 'EOF' > "$TMP/tests/3.json"
{
  "target": "https://example.com",
  "tests": [
    {
      "description": "Invalid type",
      "valid": false,
      "data": []
    }
  ]
}
EOF

"$1" test "$TMP/tests" --resolve "$TMP/schema.json" 1> "$TMP/output.txt" 2>&1 \
  && CODE="$?" || CODE="$?"
test "$CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP")/tests/1.json:
  2/2 FAIL First failure

error: Schema validation failure
  The value was expected to be of type string but it was of type integer
    at instance location ""
    at evaluate path "/type"
$(realpath "$TMP")/tests/2.json: PASS 1/1
$(realpath "$TMP")/tests/3.json: PASS 1/1
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
