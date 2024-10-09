#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/document.json"
{ "version": 2.0 }
EOF

"$1" encode "$TMP/document.json" "$TMP/output.binpack"
"$1" decode "$TMP/output.binpack" "$TMP/decode.json"

cat << 'EOF' > "$TMP/expected.json"
{
  "version": 2.0
}
EOF

diff "$TMP/decode.json" "$TMP/expected.json"
