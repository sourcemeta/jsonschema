#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com",
  "$ref": "#/$defs/string",
  "$defs": {
    "string": { "type": "string" }
  }
}
EOF

# Test with --json flag
"$1" frame "$TMP/schema.json" --json > "$TMP/result_json.txt"

# Test with -j flag
"$1" frame "$TMP/schema.json" -j > "$TMP/result.json"

cat << 'EOF' > "$TMP/expected_json.txt"
{
  "frames": {
    "https://example.com": {
      "baseURI": "https://example.com",
      "dialect": "https://json-schema.org/draft/2020-12/schema",
      "pointer": "",
      "relativePointer": "",
      "schema": "https://example.com",
      "type": "Resource"
    },
    "https://example.com#/$defs": {
      "baseURI": "https://example.com",
      "dialect": "https://json-schema.org/draft/2020-12/schema",
      "pointer": "/$defs",
      "relativePointer": "/$defs",
      "schema": "https://example.com",
      "type": "Pointer"
    },
    "https://example.com#/$defs/string": {
      "baseURI": "https://example.com",
      "dialect": "https://json-schema.org/draft/2020-12/schema",
      "pointer": "/$defs/string",
      "relativePointer": "/$defs/string",
      "schema": "https://example.com",
      "type": "Pointer"
    },
    "https://example.com#/$defs/string/type": {
      "baseURI": "https://example.com",
      "dialect": "https://json-schema.org/draft/2020-12/schema",
      "pointer": "/$defs/string/type",
      "relativePointer": "/$defs/string/type",
      "schema": "https://example.com",
      "type": "Pointer"
    },
    "https://example.com#/$id": {
      "baseURI": "https://example.com",
      "dialect": "https://json-schema.org/draft/2020-12/schema",
      "pointer": "/$id",
      "relativePointer": "/$id",
      "schema": "https://example.com",
      "type": "Pointer"
    },
    "https://example.com#/$ref": {
      "baseURI": "https://example.com",
      "dialect": "https://json-schema.org/draft/2020-12/schema",
      "pointer": "/$ref",
      "relativePointer": "/$ref",
      "schema": "https://example.com",
      "type": "Pointer"
    },
    "https://example.com#/$schema": {
      "baseURI": "https://example.com",
      "dialect": "https://json-schema.org/draft/2020-12/schema",
      "pointer": "/$schema",
      "relativePointer": "/$schema",
      "schema": "https://example.com",
      "type": "Pointer"
    }
  },
  "references": {
    "/$ref": {
      "destination": "https://example.com#/$defs/string",
      "fragment": "/$defs/string",
      "fragmentBaseURI": "https://example.com",
      "type": "Static"
    }
  }
}
EOF

diff "$TMP/result_json.txt" "$TMP/expected_json.txt"