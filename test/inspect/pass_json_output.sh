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
"$1" inspect "$TMP/schema.json" --json > "$TMP/result_json.txt"

cat << 'EOF' > "$TMP/expected_json.txt"
{
  "locations": [
    {
      "referenceType": "static",
      "uri": "https://example.com",
      "parent": null,
      "type": "resource",
      "root": "https://example.com",
      "base": "https://example.com",
      "pointer": "",
      "relativePointer": "",
      "dialect": "https://json-schema.org/draft/2020-12/schema",
      "baseDialect": "https://json-schema.org/draft/2020-12/schema"
    },
    {
      "referenceType": "static",
      "uri": "https://example.com#/$defs",
      "parent": "",
      "type": "pointer",
      "root": "https://example.com",
      "base": "https://example.com",
      "pointer": "/$defs",
      "relativePointer": "/$defs",
      "dialect": "https://json-schema.org/draft/2020-12/schema",
      "baseDialect": "https://json-schema.org/draft/2020-12/schema"
    },
    {
      "referenceType": "static",
      "uri": "https://example.com#/$defs/string",
      "parent": "",
      "type": "subschema",
      "root": "https://example.com",
      "base": "https://example.com",
      "pointer": "/$defs/string",
      "relativePointer": "/$defs/string",
      "dialect": "https://json-schema.org/draft/2020-12/schema",
      "baseDialect": "https://json-schema.org/draft/2020-12/schema"
    },
    {
      "referenceType": "static",
      "uri": "https://example.com#/$defs/string/type",
      "parent": "/$defs/string",
      "type": "pointer",
      "root": "https://example.com",
      "base": "https://example.com",
      "pointer": "/$defs/string/type",
      "relativePointer": "/$defs/string/type",
      "dialect": "https://json-schema.org/draft/2020-12/schema",
      "baseDialect": "https://json-schema.org/draft/2020-12/schema"
    },
    {
      "referenceType": "static",
      "uri": "https://example.com#/$id",
      "parent": "",
      "type": "pointer",
      "root": "https://example.com",
      "base": "https://example.com",
      "pointer": "/$id",
      "relativePointer": "/$id",
      "dialect": "https://json-schema.org/draft/2020-12/schema",
      "baseDialect": "https://json-schema.org/draft/2020-12/schema"
    },
    {
      "referenceType": "static",
      "uri": "https://example.com#/$ref",
      "parent": "",
      "type": "pointer",
      "root": "https://example.com",
      "base": "https://example.com",
      "pointer": "/$ref",
      "relativePointer": "/$ref",
      "dialect": "https://json-schema.org/draft/2020-12/schema",
      "baseDialect": "https://json-schema.org/draft/2020-12/schema"
    },
    {
      "referenceType": "static",
      "uri": "https://example.com#/$schema",
      "parent": "",
      "type": "pointer",
      "root": "https://example.com",
      "base": "https://example.com",
      "pointer": "/$schema",
      "relativePointer": "/$schema",
      "dialect": "https://json-schema.org/draft/2020-12/schema",
      "baseDialect": "https://json-schema.org/draft/2020-12/schema"
    }
  ],
  "references": [
    {
      "type": "static",
      "origin": "/$ref",
      "destination": "https://example.com#/$defs/string",
      "base": "https://example.com",
      "fragment": "/$defs/string"
    },
    {
      "type": "static",
      "origin": "/$schema",
      "destination": "https://json-schema.org/draft/2020-12/schema",
      "base": "https://json-schema.org/draft/2020-12/schema",
      "fragment": null
    }
  ],
  "instances": {
    "": [ "" ],
    "/$defs/string": [ "" ]
  }
}
EOF

diff "$TMP/result_json.txt" "$TMP/expected_json.txt"
