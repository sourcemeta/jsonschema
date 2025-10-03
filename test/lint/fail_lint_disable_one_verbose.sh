#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string",
  "contentMediaType": "application/json",
  "enum": [ "foo" ]
}
EOF

cd "$TMP"
"$1" lint "$TMP/schema.json" --verbose --exclude enum_to_const >"$TMP/stderr.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
Disabling rule: enum_to_const
Linting: $(realpath "$TMP")/schema.json
schema.json:4:3:
  The \`contentMediaType\` keyword is meaningless without the presence of the \`contentEncoding\` keyword (content_media_type_without_encoding)
    at schema location "/contentMediaType"
schema.json:5:3:
  Setting \`type\` alongside \`enum\` is considered an anti-pattern, as the enumeration choices already imply their respective types (enum_with_type)
    at schema location "/enum"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
