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

"$1" encode "$TMP/document.jsonl" "$TMP/output.binpack"
"$1" decode "$TMP/output.binpack" "$TMP/result.jsonl" --verbose > "$TMP/output.txt" 2>&1

cat "$TMP/result.jsonl"

cat << EOF > "$TMP/expected.jsonl"
{
  "count": 1
}
{
  "count": 2
}
{
  "count": 3
}
{
  "count": 4
}
{
  "count": 5
}
EOF

cat << EOF > "$TMP/expected-output.txt"
Interpreting input as JSONL: $(realpath "$TMP")/output.binpack
Decoding entry #0
Decoding entry #1
Decoding entry #2
Decoding entry #3
Decoding entry #4
EOF

diff "$TMP/expected.jsonl" "$TMP/result.jsonl"
diff "$TMP/output.txt" "$TMP/expected-output.txt"
