#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "properties": {
    "foo": {
      "type": "string"
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "foo": 1 }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" --trace > "$TMP/output.txt" \
  && CODE="$?" || CODE="$?"
test "$CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
-> (push) "/properties" (LogicalAnd)
   at ""
   at keyword location "#/properties"

-> (push) "/properties/foo/type" (AssertionTypeStrict)
   at "/foo"
   at keyword location "#/properties/foo/type"

<- (fail) "/properties/foo/type" (AssertionTypeStrict)
   at "/foo"
   at keyword location "#/properties/foo/type"

<- (fail) "/properties" (LogicalAnd)
   at ""
   at keyword location "#/properties"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
