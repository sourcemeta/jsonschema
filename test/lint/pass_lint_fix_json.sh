#!/bin/sh
set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-06/schema#",
  "type": "string",
  "enum": [ "foo" ]
}
EOF

"$1" lint "$TMP/schema.json" --fix --json >"$TMP/output.json" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "0" || exit 1

cat << EOF > "$TMP/expected.json"
{
  "valid": true,
  "errors": []
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"

cat << 'EOF' > "$TMP/expected_fixed.json"
{
  "$schema": "http://json-schema.org/draft-06/schema#",
  "const": "foo"
}
EOF

diff "$TMP/schema.json" "$TMP/expected_fixed.json"

echo "pass_lint_fix_json: PASS"
