#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir "$TMP/schemas"

cat << 'EOF' > "$TMP/schemas/1.json"
{
  "id": "https://example.com/1",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/schemas/2.json"
{
  "id": "https://example.com/2",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "array"
}
EOF

mkdir "$TMP/tests"

cat << 'EOF' > "$TMP/tests/1.json"
{
  "target": "https://example.com/1",
  "tests": [
    {
      "description": "First test",
      "valid": true,
      "data": "foo"
    },
    {
      "description": "Second test",
      "valid": false,
      "data": 1
    }
  ]
}
EOF

cat << 'EOF' > "$TMP/tests/2.json"
{
  "target": "https://example.com/2",
  "tests": [
    {
      "description": "First test",
      "valid": true,
      "data": [ 1, 2, 3 ]
    },
    {
      "description": "Second test",
      "valid": false,
      "data": {}
    }
  ]
}
EOF

"$1" test "$TMP/tests" --resolve "$TMP/schemas" --verbose 1> "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Detecting schema resources from file: $(realpath "$TMP")/schemas/1.json
Importing schema into the resolution context: file://$(realpath "$TMP")/schemas/1.json
Importing schema into the resolution context: https://example.com/1
Detecting schema resources from file: $(realpath "$TMP")/schemas/2.json
Importing schema into the resolution context: file://$(realpath "$TMP")/schemas/2.json
Importing schema into the resolution context: https://example.com/2
Looking for target: https://example.com/1
$(realpath "$TMP")/tests/1.json:
  1/2 PASS First test
  2/2 PASS Second test
Looking for target: https://example.com/2
$(realpath "$TMP")/tests/2.json:
  1/2 PASS First test
  2/2 PASS Second test
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
