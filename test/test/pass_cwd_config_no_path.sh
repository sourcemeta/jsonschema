#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/project"

cat << 'EOF' > "$TMP/schema.json"
{
  "id": "https://example.com",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/project/test_suite.json"
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

cat << 'EOF' > "$TMP/project/jsonschema.json"
{
  "defaultDialect": "http://json-schema.org/draft-04/schema#",
  "ignore": [
    "./jsonschema.json"
  ]
}
EOF

cd "$TMP/project"
"$1" test --resolve "$TMP/schema.json" > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
warning: Recursively processing every file in $(realpath "$TMP/project") as the configuration file does not set an explicit path
$(realpath "$TMP/project")/test_suite.json: PASS 1/1
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
