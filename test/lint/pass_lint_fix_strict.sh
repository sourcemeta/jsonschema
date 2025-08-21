#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-06/schema#",
  "required": [ "foo", "bar" ]
}
EOF

"$1" lint "$TMP/schema.json" --fix --strict

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "http://json-schema.org/draft-06/schema#",
  "required": [ "foo", "bar" ],
  "properties": {
    "foo": true,
    "bar": true
  }
}
EOF

diff "$TMP/schema.json" "$TMP/expected.json"
