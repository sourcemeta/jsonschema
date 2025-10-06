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

"$1" fmt "$TMP/schema.json" --check

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": 1
}
EOF

diff "$TMP/schema.json" "$TMP/expected.json"
