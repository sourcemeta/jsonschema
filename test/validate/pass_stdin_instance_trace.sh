#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

echo '"hello"' | "$1" validate "$TMP/schema.json" - --trace > "$TMP/output.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "0" || exit 1

SCHEMA_PATH="$(cd "$TMP" && pwd -P)/schema.json"

cat << EOF > "$TMP/expected.txt"
-> (push) "/type" (AssertionTypeStrict)
   at instance location "" (line 1, column 1)
   at keyword location "file://$SCHEMA_PATH#/type"
   at vocabulary "https://json-schema.org/draft/2020-12/vocab/validation"

<- (pass) "/type" (AssertionTypeStrict)
   at instance location "" (line 1, column 1)
   at keyword location "file://$SCHEMA_PATH#/type"
   at vocabulary "https://json-schema.org/draft/2020-12/vocab/validation"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
