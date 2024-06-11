#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-06/schema#",
  "type": "string",
  "enum": [ "foo" ]
}
EOF

"$1" lint "$TMP/schema.json" --fix

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "http://json-schema.org/draft-06/schema#",
  "const": "foo"
}
EOF

diff "$TMP/schema.json" "$TMP/expected.json"
