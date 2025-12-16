#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "type": 1,
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema"
}
EOF

"$1" fmt "$TMP/schema.json" --check --keep-ordering >"$TMP/output.txt" 2>&1

cat << 'EOF' > "$TMP/expected_output.txt"
EOF

diff "$TMP/output.txt" "$TMP/expected_output.txt"

cat << 'EOF' > "$TMP/expected.json"
{
  "type": 1,
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema"
}
EOF

diff "$TMP/schema.json" "$TMP/expected.json"
