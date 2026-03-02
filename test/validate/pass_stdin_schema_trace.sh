#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/instance.json"
"hello"
EOF

cat << 'EOF' | "$1" validate - "$TMP/instance.json" --trace > "$TMP/output.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/test-schema",
  "type": "string"
}
EOF
test "$EXIT_CODE" = "0" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
-> (push) "/type" (AssertionTypeStrict)
   at instance location "" (line 1, column 1)
   at keyword location "https://example.com/test-schema#/type"
   at vocabulary "https://json-schema.org/draft/2020-12/vocab/validation"

<- (pass) "/type" (AssertionTypeStrict)
   at instance location "" (line 1, column 1)
   at keyword location "https://example.com/test-schema#/type"
   at vocabulary "https://json-schema.org/draft/2020-12/vocab/validation"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
