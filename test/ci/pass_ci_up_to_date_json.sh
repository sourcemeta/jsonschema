#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/source" "$TMP/project"

cat << 'EOF' > "$TMP/source/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

cat << EOF > "$TMP/project/jsonschema.json"
{
  "dependencies": {
    "file://$(realpath "$TMP")/source/schema.json": "./vendor/schema.json"
  }
}
EOF

cd "$TMP/project"
"$1" install > /dev/null 2>&1

cp "$TMP/project/jsonschema.lock.json" "$TMP/lock_before.json"

"$1" ci --json > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
{
  "events": [
    {
      "type": "up-to-date",
      "uri": "file://$(realpath "$TMP")/source/schema.json"
    }
  ]
}
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

diff "$TMP/project/jsonschema.lock.json" "$TMP/lock_before.json"
