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
  "additionalProperties": true
}
EOF

cd "$TMP"
"$1" lint "$TMP/schema.json" >"$TMP/stderr.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
schema.json:4:27:
  Setting the `additionalProperties` keyword to the true schema does not add any further constraint (additional_properties_default)
    at schema location "/additionalProperties"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
