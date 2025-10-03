#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://example.com/meta",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/meta.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/meta",
  "$vocabulary": {
    "https://json-schema.org/draft/2020-12/vocab/core": true,
    "https://json-schema.org/draft/2020-12/vocab/applicator": true,
    "https://json-schema.org/draft/2020-12/vocab/unevaluated": true,
    "https://json-schema.org/draft/2020-12/vocab/validation": true,
    "https://json-schema.org/draft/2020-12/vocab/meta-data": true,
    "https://json-schema.org/draft/2020-12/vocab/format-annotation": true,
    "https://json-schema.org/draft/2020-12/vocab/content": true
  },
  "$dynamicAnchor": "meta",
  "type": "object"
}
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "resolve": {
    "https://example.com/meta": "./meta.json"
  }
}
EOF

"$1" metaschema "$TMP/schema.json" --verbose 2> "$TMP/output.txt"

cat << EOF > "$TMP/expected.txt"
Using configuration file: $(realpath "$TMP")/jsonschema.json
Resolving https://example.com/meta as file://$(realpath "$TMP")/meta.json given the configuration file
Attempting to read file reference from disk: $(realpath "$TMP")/meta.json
ok: $(realpath "$TMP")/schema.json
  matches https://example.com/meta
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
