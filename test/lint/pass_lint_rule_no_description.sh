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
  "required": [ "type" ]
}
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema"
}
EOF

cd "$TMP"
"$1" lint --rule "$TMP/rule.json" --only require_type "$TMP/schema.json" \
  > "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "2" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
schema.json:1:1:
  <no description> (require_type)
    at location ""
    The object value was expected to define the property "type"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
