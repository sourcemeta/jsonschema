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

"$1" lint "$TMP/schema.json" --exclude enum_to_const -x enum_with_type >"$TMP/stderr.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP")/schema.json:
  The \`contentMediaType\` keyword is meaningless without the presence of the \`contentEncoding\` keyword (content_media_type_without_encoding)
    at schema location ""
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
