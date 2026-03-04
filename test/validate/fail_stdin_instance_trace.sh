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

echo '42' | "$1" validate "$TMP/schema.json" - --trace > "$TMP/output.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Validation failure
test "$EXIT_CODE" = "2" || exit 1

SCHEMA_PATH="$(cd "$TMP" && pwd -P)/schema.json"

cat << EOF > "$TMP/expected.txt"
-> (push) "/type" (AssertionTypeStrict)
   at instance location "" (line 1, column 1)
   at keyword location "file://$SCHEMA_PATH#/type"
   at vocabulary "https://json-schema.org/draft/2020-12/vocab/validation"

<- (fail) "/type" (AssertionTypeStrict)
   at instance location "" (line 1, column 1)
   at keyword location "file://$SCHEMA_PATH#/type"
   at vocabulary "https://json-schema.org/draft/2020-12/vocab/validation"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

# JSON error
echo '42' | "$1" validate "$TMP/schema.json" - --json >"$TMP/stdout.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.json"
<stdin>
{
  "valid": false,
  "errors": [
    {
      "keywordLocation": "/type",
      "absoluteKeywordLocation": "file://$SCHEMA_PATH#/type",
      "instanceLocation": "",
      "instancePosition": [ 1, 1, 1, 2 ],
      "error": "The value was expected to be of type string but it was of type integer"
    }
  ]
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.json"
