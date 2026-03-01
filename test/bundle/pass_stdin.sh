#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' | "$1" bundle - >"$TMP/output.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/test",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/test",
  "type": "string"
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
