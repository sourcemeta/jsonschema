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

cat << 'EOF' > "$TMP/data-valid.json"
"Hello World"
EOF

cat << 'EOF' > "$TMP/data-invalid.json"
{ "type": "Hello World" }
EOF

cat << 'EOF' > "$TMP/test.json"
{
  "target": "https://example.com",
  "tests": [
    {
      "description": "First test",
      "valid": true,
      "dataPath": "./data-valid.json"
    },
    {
      "description": "Second test",
      "valid": false,
      "dataPath": "./data-invalid.json"
    }
  ]
}
EOF

"$1" test "$TMP/test.json" --resolve "$TMP/schema.json" --verbose 1> "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Detecting schema resources from file: $(realpath "$TMP")/schema.json
Importing schema into the resolution context: file://$(realpath "$TMP")/schema.json
Importing schema into the resolution context: https://example.com
$(realpath "$TMP")/test.json:
Reading test instance file: $(realpath "$TMP")/data-valid.json
  1/2 PASS First test
Reading test instance file: $(realpath "$TMP")/data-invalid.json
  2/2 PASS Second test
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
