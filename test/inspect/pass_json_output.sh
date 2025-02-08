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
  "frames": {
    "https://example.com": {
      "base": "https://example.com",
      "dialect": "https://json-schema.org/draft/2020-12/schema",
      "pointer": "",
      "relativePointer": "",
      "root": "https://example.com",
      "type": "resource"
    },
    "https://example.com#/$defs": {
      "base": "https://example.com",
      "dialect": "https://json-schema.org/draft/2020-12/schema",
      "pointer": "/$defs",
      "relativePointer": "/$defs",
      "root": "https://example.com",
      "type": "pointer"
    },
    "https://example.com#/$defs/string": {
      "base": "https://example.com",
      "dialect": "https://json-schema.org/draft/2020-12/schema",
      "pointer": "/$defs/string",
      "relativePointer": "/$defs/string",
      "root": "https://example.com",
      "type": "pointer"
    },
    "https://example.com#/$defs/string/type": {
      "base": "https://example.com",
      "dialect": "https://json-schema.org/draft/2020-12/schema",
      "pointer": "/$defs/string/type",
      "relativePointer": "/$defs/string/type",
      "root": "https://example.com",
      "type": "pointer"
    },
    "https://example.com#/$id": {
      "base": "https://example.com",
      "dialect": "https://json-schema.org/draft/2020-12/schema",
      "pointer": "/$id",
      "relativePointer": "/$id",
      "root": "https://example.com",
      "type": "pointer"
    },
    "https://example.com#/$ref": {
      "base": "https://example.com",
      "dialect": "https://json-schema.org/draft/2020-12/schema",
      "pointer": "/$ref",
      "relativePointer": "/$ref",
      "root": "https://example.com",
      "type": "pointer"
    },
    "https://example.com#/$schema": {
      "base": "https://example.com",
      "dialect": "https://json-schema.org/draft/2020-12/schema",
      "pointer": "/$schema",
      "relativePointer": "/$schema",
      "root": "https://example.com",
      "type": "pointer"
    }
  },
  "references": {
    "/$ref": {
      "base": "https://example.com",
      "destination": "https://example.com#/$defs/string",
      "fragment": "/$defs/string",
      "type": "static"
    },
    "/$schema": {
      "base": "https://json-schema.org/draft/2020-12/schema",
      "destination": "https://json-schema.org/draft/2020-12/schema",
      "fragment": null,
      "type": "static"
    }
  }
}
EOF

diff "$TMP/result_json.txt" "$TMP/expected_json.txt"
