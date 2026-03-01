#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' | "$1" fmt - --verbose >"$TMP/output.txt" 2>&1
{"$schema":"https://json-schema.org/draft/2020-12/schema","type":"string"}
EOF

cat << 'EOF' > "$TMP/expected.txt"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
