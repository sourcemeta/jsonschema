#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' | "$1" metaschema - --verbose 2>"$TMP/stderr.txt"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/expected.txt"
ok: <stdin>
  matches https://json-schema.org/draft/2020-12/schema
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
