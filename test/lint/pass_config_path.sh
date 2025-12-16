#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/foo"
mkdir -p "$TMP/bar"

cat << 'EOF' > "$TMP/foo/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema",
  "properties": {
    "foo": {
      "type": "string",
      "default": "foo"
    }
  }
}
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "path": "./foo"
}
EOF

cd "$TMP/bar"
"$1" lint --verbose > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Using configuration file: $(realpath "$TMP")/jsonschema.json
Using extension: .json
Using extension: .yaml
Using extension: .yml
Linting: $(realpath "$TMP")/foo/schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema",
  "properties": {
    "foo": {
      "type": "string",
      "default": "foo"
    }
  }
}
EOF

diff "$TMP/foo/schema.json" "$TMP/expected.json"
