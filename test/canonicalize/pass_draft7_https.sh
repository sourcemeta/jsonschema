#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft-07/schema#",
  "type": "string"
}
EOF

"$1" canonicalize "$TMP/schema.json" > "$TMP/result.json" 2> "$TMP/stderr.txt"

cat << EOF > "$TMP/expected_stderr.txt"
EOF

diff "$TMP/stderr.txt" "$TMP/expected_stderr.txt"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "string",
  "minLength": 0
}
EOF

diff "$TMP/result.json" "$TMP/expected.json"
