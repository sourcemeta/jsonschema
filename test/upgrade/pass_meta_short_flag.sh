#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "$id": "https://example.com/my-metaschema",
  "title": "My Custom Meta-Schema",
  "type": [ "object", "boolean" ]
}
EOF

"$1" upgrade -m -t 2019-09 "$TMP/schema.json" > "$TMP/output_short.json"
"$1" upgrade --meta --to 2019-09 "$TMP/schema.json" > "$TMP/output_long.json"

diff "$TMP/output_short.json" "$TMP/output_long.json"
