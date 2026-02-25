#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' | "$1" fmt --check - --json >"$TMP/output.json" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
{"$schema":"https://json-schema.org/draft/2020-12/schema","type":"string"}
EOF
test "$EXIT_CODE" = "2" || exit 1

cat << 'EOF' > "$TMP/expected.json"
{
  "valid": false,
  "errors": [ "(stdin)" ]
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
