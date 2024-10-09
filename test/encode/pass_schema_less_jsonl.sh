#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/document.jsonl"
{ "count": 1 }
{ "count": 2 }
{ "count": 3 }
{ "count": 4 }
{ "count": 5 }
EOF

"$1" encode "$TMP/document.jsonl" "$TMP/output.binpack" > "$TMP/output.txt" 2>&1
xxd "$TMP/output.binpack" > "$TMP/output.hex"

cat << 'EOF' > "$TMP/expected.txt"
00000000: 1306 636f 756e 7415 1300 091d 1300 0525  ..count........%
00000010: 1300 052d 1300 0535                      ...-...5
EOF

cat << 'EOF' > "$TMP/expected-output.txt"
original file size: 75 bytes
encoded file size: 24 bytes
compression ratio: 32%
EOF

diff "$TMP/expected.txt" "$TMP/output.hex"
diff "$TMP/output.txt" "$TMP/expected-output.txt"
