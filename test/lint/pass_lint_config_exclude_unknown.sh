#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "lint": {
    "exclude": [ "this_rule_does_not_exist" ]
  }
}
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test schema",
  "examples": [ "foo" ],
  "type": "string"
}
EOF

cd "$TMP"
"$1" lint --verbose "$TMP/schema.json" > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Using extension: .json
Using extension: .yaml
Using extension: .yml
Linting: $(realpath "$TMP")/schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
