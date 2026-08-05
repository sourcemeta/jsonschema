#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "lint": {
    "exclude": [ "enum_with_type" ]
  }
}
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test schema",
  "examples": [ "foo" ],
  "type": "string",
  "enum": [ "foo", "bar" ]
}
EOF

cd "$TMP"
"$1" lint --only enum_with_type "$TMP/schema.json" \
  > "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Lint violation
test "$EXIT_CODE" = "2"

cat << 'EOF' > "$TMP/expected.txt"
schema.json:7:3:
  Setting `type` alongside `enum` is considered an anti-pattern, as the enumeration choices already imply their respective types (enum_with_type)
    at location "/enum"
schema.json:6:3:
  Setting `type` alongside `enum` is considered an anti-pattern, as the enumeration choices already imply their respective types (enum_with_type)
    at location "/type"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
