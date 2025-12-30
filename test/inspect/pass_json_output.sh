#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test schema",
  "$id": "https://example.com",
  "$ref": "#/$defs/string",
  "$defs": {
    "string": { "type": "string" }
  }
}
EOF

# Test with --json flag
"$1" inspect "$TMP/schema.json" --json > "$TMP/result_json.txt"

cat << 'EOF' > "$TMP/expected_json.txt"
{
  "locations": {
    "static": {
      "https://example.com": {
        "parent": null,
        "type": "resource",
        "root": "https://example.com",
        "base": "https://example.com",
        "pointer": "",
        "position": [ 1, 1, 10, 1 ],
        "relativePointer": "",
        "dialect": "https://json-schema.org/draft/2020-12/schema",
        "baseDialect": "https://json-schema.org/draft/2020-12/schema"
      },
      "https://example.com#/$defs": {
        "parent": "",
        "type": "pointer",
        "root": "https://example.com",
        "base": "https://example.com",
        "pointer": "/$defs",
        "position": [ 7, 3, 9, 3 ],
        "relativePointer": "/$defs",
        "dialect": "https://json-schema.org/draft/2020-12/schema",
        "baseDialect": "https://json-schema.org/draft/2020-12/schema"
      },
      "https://example.com#/$defs/string": {
        "parent": "",
        "type": "subschema",
        "root": "https://example.com",
        "base": "https://example.com",
        "pointer": "/$defs/string",
        "position": [ 8, 5, 8, 34 ],
        "relativePointer": "/$defs/string",
        "dialect": "https://json-schema.org/draft/2020-12/schema",
        "baseDialect": "https://json-schema.org/draft/2020-12/schema"
      },
      "https://example.com#/$defs/string/type": {
        "parent": "/$defs/string",
        "type": "pointer",
        "root": "https://example.com",
        "base": "https://example.com",
        "pointer": "/$defs/string/type",
        "position": [ 8, 17, 8, 32 ],
        "relativePointer": "/$defs/string/type",
        "dialect": "https://json-schema.org/draft/2020-12/schema",
        "baseDialect": "https://json-schema.org/draft/2020-12/schema"
      },
      "https://example.com#/$id": {
        "parent": "",
        "type": "pointer",
        "root": "https://example.com",
        "base": "https://example.com",
        "pointer": "/$id",
        "position": [ 5, 3, 5, 30 ],
        "relativePointer": "/$id",
        "dialect": "https://json-schema.org/draft/2020-12/schema",
        "baseDialect": "https://json-schema.org/draft/2020-12/schema"
      },
      "https://example.com#/$ref": {
        "parent": "",
        "type": "pointer",
        "root": "https://example.com",
        "base": "https://example.com",
        "pointer": "/$ref",
        "position": [ 6, 3, 6, 26 ],
        "relativePointer": "/$ref",
        "dialect": "https://json-schema.org/draft/2020-12/schema",
        "baseDialect": "https://json-schema.org/draft/2020-12/schema"
      },
      "https://example.com#/$schema": {
        "parent": "",
        "type": "pointer",
        "root": "https://example.com",
        "base": "https://example.com",
        "pointer": "/$schema",
        "position": [ 2, 3, 2, 59 ],
        "relativePointer": "/$schema",
        "dialect": "https://json-schema.org/draft/2020-12/schema",
        "baseDialect": "https://json-schema.org/draft/2020-12/schema"
      },
      "https://example.com#/description": {
        "parent": "",
        "type": "pointer",
        "root": "https://example.com",
        "base": "https://example.com",
        "pointer": "/description",
        "position": [ 4, 3, 4, 30 ],
        "relativePointer": "/description",
        "dialect": "https://json-schema.org/draft/2020-12/schema",
        "baseDialect": "https://json-schema.org/draft/2020-12/schema"
      },
      "https://example.com#/title": {
        "parent": "",
        "type": "pointer",
        "root": "https://example.com",
        "base": "https://example.com",
        "pointer": "/title",
        "position": [ 3, 3, 3, 17 ],
        "relativePointer": "/title",
        "dialect": "https://json-schema.org/draft/2020-12/schema",
        "baseDialect": "https://json-schema.org/draft/2020-12/schema"
      }
    },
    "dynamic": {}
  },
  "references": [
    {
      "type": "static",
      "origin": "/$ref",
      "position": [ 6, 3, 6, 26 ],
      "destination": "https://example.com#/$defs/string",
      "base": "https://example.com",
      "fragment": "/$defs/string"
    },
    {
      "type": "static",
      "origin": "/$schema",
      "position": [ 2, 3, 2, 59 ],
      "destination": "https://json-schema.org/draft/2020-12/schema",
      "base": "https://json-schema.org/draft/2020-12/schema",
      "fragment": null
    }
  ]
}
EOF

diff "$TMP/result_json.txt" "$TMP/expected_json.txt"
