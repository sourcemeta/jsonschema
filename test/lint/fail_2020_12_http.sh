#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test schema",
  "examples": [ "foo" ],
  "type": "string"
}
EOF

cd "$TMP"
"$1" lint "$TMP/schema.json" > "$TMP/stderr.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "2"

cat << EOF > "$TMP/expected.txt"
schema.json:2:3:
  The official dialect URI of 2019-09 and later must use "https://" instead of "http://" (modern_official_dialect_with_http)
    at location "/\$schema"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
