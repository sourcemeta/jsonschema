#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/rules"

cat << 'EOF' > "$TMP/rules/jsonschema.json"
{
  "defaultDialect": "https://json-schema.org/draft/2020-12/schema"
}
EOF

cat << 'EOF' > "$TMP/rules/rule.json"
{
  "title": "require_type",
  "description": "Every subschema must declare the type keyword",
  "required": [ "type" ]
}
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

"$1" lint --rule "$TMP/rules/rule.json" --only require_type "$TMP/schema.json" \
  >"$TMP/output.txt" 2>&1

cat << 'EOF' > "$TMP/expected.txt"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
