#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/other.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/other",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "properties": {
    "local": { "$ref": "#/$defs/string" },
    "anchored": { "$ref": "#thing" },
    "external": { "$ref": "https://example.com/other" }
  },
  "$defs": {
    "string": { "$anchor": "thing", "type": "string" }
  }
}
EOF

# Removing identifiers must not leave references dangling against the
# identifier the schema was assigned for being read from standard input
"$1" bundle - --without-id --resolve "$TMP/other.json" < "$TMP/schema.json" \
  > "$TMP/output.json" 2> "$TMP/stderr.txt"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "properties": {
    "local": {
      "$ref": "#/$defs/string"
    },
    "anchored": {
      "$ref": "#/$defs/string"
    },
    "external": {
      "$ref": "#/$defs/https:~1~1example.com~1other"
    }
  },
  "$defs": {
    "string": {
      "type": "string"
    },
    "https://example.com/other": {
      "$schema": "https://json-schema.org/draft/2020-12/schema",
      "type": "string"
    }
  }
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
