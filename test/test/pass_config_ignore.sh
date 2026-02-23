#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/tests"
mkdir -p "$TMP/tests/ignored"

cat << 'EOF' > "$TMP/schema.json"
{
  "id": "https://example.com",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/tests/good.json"
{
  "target": "https://example.com",
  "tests": [
    {
      "description": "A string is valid",
      "valid": true,
      "data": "foo"
    }
  ]
}
EOF

cat << 'EOF' > "$TMP/tests/ignored/bad.json"
{
  "target": "https://example.com",
  "tests": [
    {
      "description": "A number is not valid",
      "valid": true,
      "data": 42
    }
  ]
}
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "path": "./tests",
  "ignore": [
    "./tests/ignored"
  ]
}
EOF

cd "$TMP"
"$1" test --resolve "$TMP/schema.json" > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP")/tests/good.json: PASS 1/1
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
