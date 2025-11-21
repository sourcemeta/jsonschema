#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": 1
}
EOF

"$1" fmt "$TMP/schema.json" --check --verbose >"$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected_output.txt"
Checking: $(realpath "$TMP")/schema.json
ok: $(realpath "$TMP")/schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected_output.txt"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": 1
}
EOF

diff "$TMP/schema.json" "$TMP/expected.json"
