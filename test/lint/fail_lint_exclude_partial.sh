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
  "examples": [ "foo" ],
  "type": "string",
  "enum": [ "foo" ],
  "contentMediaType": "application/json",
  "x-lint-exclude": [ "enum_with_type", "enum_to_const" ]
}
EOF

cd "$TMP"
"$1" lint "$TMP/schema.json" > "$TMP/output.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
schema.json:8:3:
  The \`contentMediaType\` keyword is meaningless without the presence of the \`contentEncoding\` keyword (content_media_type_without_encoding)
    at location "/contentMediaType"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
