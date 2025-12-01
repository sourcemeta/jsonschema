#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
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
"$1" lint "$TMP/schema.json" --verbose \
  >"$TMP/stderr.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
Using configuration file: $(realpath "$TMP")/jsonschema.json
Linting: $(realpath "$TMP")/schema.json
schema.json:3:3:
  Setting \`type\` alongside \`enum\` is considered an anti-pattern, as the enumeration choices already imply their respective types (enum_with_type)
    at location "/enum"
schema.json:2:3:
  Setting \`type\` alongside \`enum\` is considered an anti-pattern, as the enumeration choices already imply their respective types (enum_with_type)
    at location "/type"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
