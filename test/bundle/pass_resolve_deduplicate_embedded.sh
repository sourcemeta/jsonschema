#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/common.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/common",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/a.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/a",
  "$ref": "common",
  "$defs": {
    "https://example.com/common": {
      "$schema": "https://json-schema.org/draft/2020-12/schema",
      "$id": "https://example.com/common",
      "type": "string"
    }
  }
}
EOF

cat << 'EOF' > "$TMP/b.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/b",
  "$ref": "common",
  "$defs": {
    "https://example.com/common": {
      "$schema": "https://json-schema.org/draft/2020-12/schema",
      "$id": "https://example.com/common",
      "type": "string"
    }
  }
}
EOF

cat << 'EOF' > "$TMP/entry.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/entry",
  "allOf": [
    { "$ref": "a" },
    { "$ref": "b" }
  ]
}
EOF

"$1" bundle "$TMP/entry.json" \
  --resolve "$TMP/a.json" \
  --resolve "$TMP/b.json" > "$TMP/result.json"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/entry",
  "allOf": [
    {
      "$ref": "a"
    },
    {
      "$ref": "b"
    }
  ],
  "$defs": {
    "https://example.com/common": {
      "$schema": "https://json-schema.org/draft/2020-12/schema",
      "$id": "https://example.com/common",
      "type": "string"
    },
    "https://example.com/a": {
      "$schema": "https://json-schema.org/draft/2020-12/schema",
      "$id": "https://example.com/a",
      "$ref": "common"
    },
    "https://example.com/b": {
      "$schema": "https://json-schema.org/draft/2020-12/schema",
      "$id": "https://example.com/b",
      "$ref": "common"
    }
  }
}
EOF

diff "$TMP/result.json" "$TMP/expected.json"
