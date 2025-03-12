#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "enum": [ "foo" ]
}
EOF

"$1" lint "$TMP/schema.json" --json >"$TMP/output.json" 2>&1

cat << EOF > "$TMP/expected.json"
{
  "valid": true,
  "errors": []
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
