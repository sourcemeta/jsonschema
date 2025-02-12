#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
cleanup() { rm -rf "$TMP"; }
trap cleanup EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string",
  "enum": [ "foo" ]
}
EOF

"$1" lint "$TMP/schema.json" --fix > "$TMP/result_first.txt" 2>&1

cat << 'EOF' > "$TMP/expected_first.txt"
EOF
diff "$TMP/result_first.txt" "$TMP/expected_first.txt"

cat << 'EOF' > "$TMP/fixed_schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "const": "foo"
}
EOF
diff "$TMP/schema.json" "$TMP/fixed_schema.json"

"$1" lint "$TMP/schema.json" --fix > "$TMP/result_second.txt" 2>&1
diff "$TMP/result_second.txt" "$TMP/expected_first.txt"
