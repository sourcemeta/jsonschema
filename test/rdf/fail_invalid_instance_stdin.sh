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
  "x-jsonld-type": "https://schema.org/Thing"
}
EOF

echo '1' | "$1" rdf "$TMP/schema.json" - 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Test assertion failure
test "$EXIT_CODE" = "2"

cat << 'EOF' > "$TMP/expected.txt"
fail: /dev/stdin
error: Schema validation failure
  The value was expected to be of type string but it was of type integer
    at instance location "" (line 1, column 1)
    at evaluate path "/type"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
