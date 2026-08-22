#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "properties": {
    "name": { "type": "string" },
    "age": { "type": "integer" }
  }
}
EOF

"$1" inspect "$TMP/schema.json" --keywords > "$TMP/result.txt"

cat << 'EOF' > "$TMP/expected.txt"
    3 - type (https://json-schema.org/draft/2020-12/vocab/validation)
    1 - properties (https://json-schema.org/draft/2020-12/vocab/applicator)
    1 - $schema (https://json-schema.org/draft/2020-12/vocab/core)
EOF

diff "$TMP/result.txt" "$TMP/expected.txt"
