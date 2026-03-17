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

cat << 'EOF' | "$1" test - --resolve "$TMP/schema.json" 1> "$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
{
  "target": "https://example.com",
  "tests": [
    {
      "description": "Unexpected pass",
      "valid": false,
      "data": "foo"
    }
  ]
}
EOF
# Test assertion failure
test "$EXIT_CODE" = "2"

cat << 'EOF' > "$TMP/expected.txt"
/dev/stdin:
  1/1 FAIL Unexpected pass

error: Passed but was expected to fail
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
