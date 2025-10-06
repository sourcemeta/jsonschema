#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#"
  "type" 1,
}
EOF

"$1" fmt "$TMP/schema.json" --check 2>"$TMP/output.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1
