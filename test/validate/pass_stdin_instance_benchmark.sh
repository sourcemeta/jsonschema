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
    "foo": {
      "type": "string"
    }
  }
}
EOF

echo '{ "foo": "bar" }' | "$1" validate "$TMP/schema.json" - --benchmark > "$TMP/output.txt"

sed -E 's/[0-9]+\.[0-9]+/<FLOAT>/g' "$TMP/output.txt" > "$TMP/normalized.txt"

cat << 'EOF' > "$TMP/expected.txt"
/dev/stdin: PASS <FLOAT> +- <FLOAT> us (<FLOAT>)
EOF

diff "$TMP/normalized.txt" "$TMP/expected.txt"
