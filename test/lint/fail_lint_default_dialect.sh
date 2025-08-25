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

cd "$TMP"
"$1" lint "$TMP/schema.json" \
  --default-dialect "http://json-schema.org/draft-04/schema#" \
  >"$TMP/stderr.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
schema.json:3:11:
  Setting \`type\` alongside \`enum\` is considered an anti-pattern, as the enumeration choices already imply their respective types (enum_with_type)
    at schema location "/enum"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
