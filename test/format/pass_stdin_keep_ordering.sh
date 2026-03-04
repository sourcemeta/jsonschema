#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' | "$1" fmt - --keep-ordering > "$TMP/output.json"
{"b":2,"a":1,"$schema":"https://json-schema.org/draft/2020-12/schema"}
EOF

cat << 'EOF' > "$TMP/expected.json"
{
  "b": 2,
  "a": 1,
  "$schema": "https://json-schema.org/draft/2020-12/schema"
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
