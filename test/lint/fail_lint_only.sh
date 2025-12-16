#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test schema",
  "type": "string",
  "contentMediaType": "application/json",
  "enum": [ "foo", "bar" ]
}
EOF

cd "$TMP"
"$1" lint "$TMP/schema.json" >"$TMP/stderr.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
schema.json:6:3:
  The \`contentMediaType\` keyword is meaningless without the presence of the \`contentEncoding\` keyword (content_media_type_without_encoding)
    at location "/contentMediaType"
schema.json:7:3:
  Setting \`type\` alongside \`enum\` is considered an anti-pattern, as the enumeration choices already imply their respective types (enum_with_type)
    at location "/enum"
schema.json:5:3:
  Setting \`type\` alongside \`enum\` is considered an anti-pattern, as the enumeration choices already imply their respective types (enum_with_type)
    at location "/type"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

"$1" lint "$TMP/schema.json" --only content_media_type_without_encoding >"$TMP/stderr.txt" 2>&1 && CODE="$?" || CODE="$?"

cat << EOF > "$TMP/expected.txt"
schema.json:6:3:
  The \`contentMediaType\` keyword is meaningless without the presence of the \`contentEncoding\` keyword (content_media_type_without_encoding)
    at location "/contentMediaType"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
