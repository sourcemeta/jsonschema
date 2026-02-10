#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/source" "$TMP/project/vendor"

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

echo "NOT VALID JSON" > "$TMP/project/jsonschema.lock.json"

cd "$TMP/project"
"$1" install --json > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
{
  "events": [
    {
      "type": "warning",
      "message": "Ignoring corrupted lock file"
    },
    {
      "type": "fetching",
      "uri": "file://$(realpath "$TMP")/source/schema.json"
    },
    {
      "type": "installed",
      "uri": "file://$(realpath "$TMP")/source/schema.json",
      "path": "$(realpath "$TMP")/project/vendor/schema.json"
    }
  ]
}
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
