#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/openapi.json"
{
  "openapi": "3.0.0",
  "info": {
    "title": "Test API",
    "version": "1.0.0"
  },
  "components": {
    "schemas": {
      "User": {
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "type": "object"
      }
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "name": "John", "age": 30 }
EOF

"$1" validate "$TMP/openapi.json" "$TMP/instance.json" \
  --path "/components/schemas/User"
