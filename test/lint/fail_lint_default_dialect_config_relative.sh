#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "title": "Test",
  "description": "Test schema",
  "type": "string",
  "enum": [ "foo" ]
}
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "defaultDialect": "http://json-schema.org/draft-04/schema#"
}
EOF

cd "$TMP"
"$1" lint schema.json --verbose \
  >"$TMP/stderr.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Lint violation
test "$EXIT_CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
Linting: $(realpath "$TMP")/schema.json
schema.json:5:3:
  Setting \`type\` alongside \`enum\` is considered an anti-pattern, as the enumeration choices already imply their respective types (enum_with_type)
    at location "/enum"
schema.json:4:3:
  Setting \`type\` alongside \`enum\` is considered an anti-pattern, as the enumeration choices already imply their respective types (enum_with_type)
    at location "/type"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
