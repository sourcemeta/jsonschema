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

"$1" lint --rule "$TMP/rule.json" --list --only require_type \
  > "$TMP/output.txt" 2>&1

cat << 'EOF' > "$TMP/expected.txt"
require_type
  Every subschema must declare the type keyword

Number of rules: 1
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
