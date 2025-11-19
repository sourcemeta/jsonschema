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

"$1" validate "$TMP/schema.json" "$TMP/instance.json" --trace --fast > "$TMP/output.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
-> (push) "/properties/foo/type" (AssertionPropertyTypeStrict)
   at instance location "/foo" (line 1, column 3)
   at keyword location "file://$(realpath "$TMP")/schema.json#/properties/foo/type"
   at vocabulary "https://json-schema.org/draft/2020-12/vocab/validation"

<- (fail) "/properties/foo/type" (AssertionPropertyTypeStrict)
   at instance location "/foo" (line 1, column 3)
   at keyword location "file://$(realpath "$TMP")/schema.json#/properties/foo/type"
   at vocabulary "https://json-schema.org/draft/2020-12/vocab/validation"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
