#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com",
  "oneOf": [
    { "required": [ "foo" ] },
    { "required": [ "bar" ] },
    { "required": [ "baz" ] }
  ]
}
EOF

cat << 'EOF' > "$TMP/instance.json"
[ { "foo": 1 } ]
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" --trace > "$TMP/output.txt" \
  && CODE="$?" || CODE="$?"
test "$CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
-> (push) "/oneOf" (LogicalXor)
   at ""
   at keyword location "https://example.com#/oneOf"
   at vocabulary "https://json-schema.org/draft/2020-12/vocab/applicator"

<- (fail) "/oneOf" (LogicalXor)
   at ""
   at keyword location "https://example.com#/oneOf"
   at vocabulary "https://json-schema.org/draft/2020-12/vocab/applicator"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
