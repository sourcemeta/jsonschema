#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/rule.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "require_id",
  "description": "The root schema must declare an $id",
  "required": [ "$id" ]
}
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/schema",
  "type": "string"
}
EOF

"$1" lint --top-level-rule "$TMP/rule.json" --only require_id --verbose "$TMP/schema.json" \
  > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Loading custom rule: $(realpath "$TMP")/rule.json
Only enabling rule: require_id
Linting: $(realpath "$TMP")/schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
