#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir "$TMP/project"

cat << 'EOF' > "$TMP/project/jsonschema.json"
{
  "dependencies": {}
}
EOF

cd "$TMP/project"
"$1" ci --json > "$TMP/output.txt" 2>&1

cat << 'EOF' > "$TMP/expected.txt"
{
  "events": []
}
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
