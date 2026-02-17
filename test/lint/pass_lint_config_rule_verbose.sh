#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/rule.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "require_type",
  "description": "Every subschema must declare the type keyword",
  "required": [ "type" ]
}
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "lint": {
    "rules": [
      "./rule.json"
    ]
  }
}
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

cd "$TMP"
"$1" lint --only require_type --verbose "$TMP/schema.json" \
  > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Loading custom rule from configuration: $(realpath "$TMP")/rule.json
Only enabling rule: require_type
Linting: $(realpath "$TMP")/schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
