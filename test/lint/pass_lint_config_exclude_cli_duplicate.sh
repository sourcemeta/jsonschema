#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "lint": {
    "exclude": [ "enum_with_type" ]
  }
}
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test schema",
  "examples": [ "foo" ],
  "type": "string",
  "enum": [ "foo", "bar" ]
}
EOF

cd "$TMP"
"$1" lint --verbose --exclude enum_with_type "$TMP/schema.json" \
  > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Disabling rule: enum_with_type
Using extension: .json
Using extension: .yaml
Using extension: .yml
Linting: $(realpath "$TMP")/schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
