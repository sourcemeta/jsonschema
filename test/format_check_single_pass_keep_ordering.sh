#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "type": 1,
  "$schema": "http://json-schema.org/draft-04/schema#"
}
EOF

"$1" fmt "$TMP/schema.json" --check --keep-ordering

cat << 'EOF' > "$TMP/expected.json"
{
  "type": 1,
  "$schema": "http://json-schema.org/draft-04/schema#"
}
EOF

diff "$TMP/schema.json" "$TMP/expected.json"
