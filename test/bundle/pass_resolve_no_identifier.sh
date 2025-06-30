#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$ref": "./nested.json"
}
EOF

cat << 'EOF' > "$TMP/nested.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

"$1" bundle "$TMP/schema.json" --resolve "$TMP/nested.json" > "$TMP/result.json"

cat << EOF > "$TMP/expected.json"
{
  "\$schema": "https://json-schema.org/draft/2020-12/schema",
  "\$ref": "./nested.json",
  "\$defs": {
    "file://$(realpath "$TMP")/nested.json": {
      "\$schema": "https://json-schema.org/draft/2020-12/schema",
      "\$id": "file://$(realpath "$TMP")/nested.json",
      "type": "string"
    }
  }
}
EOF

diff "$TMP/result.json" "$TMP/expected.json"

# Must come out formatted
"$1" fmt "$TMP/result.json" --check
