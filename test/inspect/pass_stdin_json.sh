#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' | "$1" inspect - --json >"$TMP/output.txt" 2>&1
{
  "$id": "https://example.com/test-stdin-inspect",
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/expected.txt"
{
  "locations": {
    "static": {
      "https://example.com/test-stdin-inspect": {
        "parent": null,
        "type": "resource",
        "root": "https://example.com/test-stdin-inspect",
        "base": "https://example.com/test-stdin-inspect",
        "pointer": "",
        "position": [ 1, 1, 5, 1 ],
        "relativePointer": "",
        "dialect": "https://json-schema.org/draft/2020-12/schema",
        "baseDialect": "https://json-schema.org/draft/2020-12/schema",
        "propertyName": false,
        "orphan": false
      },
      "https://example.com/test-stdin-inspect#/$id": {
        "parent": "",
        "type": "pointer",
        "root": "https://example.com/test-stdin-inspect",
        "base": "https://example.com/test-stdin-inspect",
        "pointer": "/$id",
        "position": [ 2, 3, 2, 49 ],
        "relativePointer": "/$id",
        "dialect": "https://json-schema.org/draft/2020-12/schema",
        "baseDialect": "https://json-schema.org/draft/2020-12/schema",
        "propertyName": false,
        "orphan": false
      },
      "https://example.com/test-stdin-inspect#/$schema": {
        "parent": "",
        "type": "pointer",
        "root": "https://example.com/test-stdin-inspect",
        "base": "https://example.com/test-stdin-inspect",
        "pointer": "/$schema",
        "position": [ 3, 3, 3, 59 ],
        "relativePointer": "/$schema",
        "dialect": "https://json-schema.org/draft/2020-12/schema",
        "baseDialect": "https://json-schema.org/draft/2020-12/schema",
        "propertyName": false,
        "orphan": false
      },
      "https://example.com/test-stdin-inspect#/type": {
        "parent": "",
        "type": "pointer",
        "root": "https://example.com/test-stdin-inspect",
        "base": "https://example.com/test-stdin-inspect",
        "pointer": "/type",
        "position": [ 4, 3, 4, 18 ],
        "relativePointer": "/type",
        "dialect": "https://json-schema.org/draft/2020-12/schema",
        "baseDialect": "https://json-schema.org/draft/2020-12/schema",
        "propertyName": false,
        "orphan": false
      }
    },
    "dynamic": {}
  },
  "references": [
    {
      "type": "static",
      "origin": "/$schema",
      "position": [ 3, 3, 3, 59 ],
      "destination": "https://json-schema.org/draft/2020-12/schema",
      "base": "https://json-schema.org/draft/2020-12/schema",
      "fragment": null
    }
  ]
}
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
