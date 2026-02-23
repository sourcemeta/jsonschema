#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/rule_type.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "require_type",
  "description": "Every subschema must declare the type keyword",
  "required": [ "type" ]
}
EOF

cat << 'EOF' > "$TMP/rule_description.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "require_description",
  "description": "Every subschema must declare the description keyword",
  "required": [ "description" ]
}
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string",
  "description": "A test"
}
EOF

"$1" lint --rule "$TMP/rule_type.json" --rule "$TMP/rule_description.json" \
  --only require_type --only require_description "$TMP/schema.json" \
  > "$TMP/output.txt" 2>&1

cat << 'EOF' > "$TMP/expected.txt"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
