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

"$1" lint "$TMP/schema.json" --verbose --disable foo_bar >"$TMP/stderr.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
warning: Cannot disable unknown rule: foo_bar
Linting: $(realpath "$TMP")/schema.json
$(realpath "$TMP")/schema.json:
  The \`contentMediaType\` keyword is meaningless without the presence of the \`contentEncoding\` keyword (content_media_type_without_encoding)
    at schema location ""
$(realpath "$TMP")/schema.json:
  An \`enum\` of a single value can be expressed as \`const\` (enum_to_const)
    at schema location ""
$(realpath "$TMP")/schema.json:
  Setting \`type\` alongside \`enum\` is considered an anti-pattern, as the enumeration choices already imply their respective types (enum_with_type)
    at schema location ""
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
