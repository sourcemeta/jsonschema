#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/document.json"
{ "version": 2.0 }
EOF

"$1" encode "$TMP/document.json" "$TMP/output.binpack" > "$TMP/output.txt" 2>&1
xxd "$TMP/output.binpack" > "$TMP/output.hex"

cat << 'EOF' > "$TMP/expected.txt"
00000000: 1308 7665 7273 696f 6e37 02              ..version7.
EOF

cat << 'EOF' > "$TMP/expected-output.txt"
size: 11 bytes
EOF

diff "$TMP/expected.txt" "$TMP/output.hex"
diff "$TMP/output.txt" "$TMP/expected-output.txt"
