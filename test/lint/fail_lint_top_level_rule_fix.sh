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
  "$schema": "https://json-schema.org/draft/2020-12/schema"
}
EOF

cd "$TMP"
"$1" lint --top-level-rule "$TMP/rule.json" --only require_id --fix "$TMP/schema.json" \
  > "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Lint violation
test "$EXIT_CODE" = "2"

cat << 'EOF' > "$TMP/expected.txt"
schema.json:1:1:
  The root schema must declare an $id (require_id)
    at location ""
    The object value was expected to define the property "$id"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cat << 'EOF' > "$TMP/expected_schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema"
}
EOF

diff "$TMP/schema.json" "$TMP/expected_schema.json"
