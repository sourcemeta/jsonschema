#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/foo"
mkdir -p "$TMP/bar"

cat << 'EOF' > "$TMP/schema.json"
{
  "id": "https://example.com",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/foo/test.json"
{
  "target": "https://example.com",
  "tests": []
}
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "path": "./foo"
}
EOF

cd "$TMP/bar"
"$1" test --resolve "$TMP/schema.json" --verbose 1> "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Using configuration file: $(realpath "$TMP")/jsonschema.json
Using extension: .json
Using extension: .yaml
Using extension: .yml
Detecting schema resources from file: $(realpath "$TMP")/schema.json
Importing schema into the resolution context: file://$(realpath "$TMP")/schema.json
Importing schema into the resolution context: https://example.com
Looking for target: https://example.com
$(realpath "$TMP")/foo/test.json: NO TESTS
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
