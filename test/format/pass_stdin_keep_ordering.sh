#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' | "$1" fmt - --keep-ordering > "$TMP/output.json" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
{"b":2,"a":1,"$schema":"https://json-schema.org/draft/2020-12/schema"}
EOF
test "$EXIT_CODE" = "0" || exit 1

cat << 'EOF' > "$TMP/expected.json"
{
  "b": 2,
  "a": 1,
  "$schema": "https://json-schema.org/draft/2020-12/schema"
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
