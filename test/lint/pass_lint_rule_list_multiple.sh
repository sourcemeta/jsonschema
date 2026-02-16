#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/rule_zzz.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "zzz_rule",
  "description": "Last rule",
  "required": [ "type" ]
}
EOF

cat << 'EOF' > "$TMP/rule_aaa.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "aaa_rule",
  "description": "First rule",
  "required": [ "description" ]
}
EOF

"$1" lint --rule "$TMP/rule_zzz.json" --rule "$TMP/rule_aaa.json" \
  --list --only aaa_rule --only zzz_rule \
  >"$TMP/output.txt" 2>&1

cat << 'EOF' > "$TMP/expected.txt"
aaa_rule
  First rule

zzz_rule
  Last rule

Number of rules: 2
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
