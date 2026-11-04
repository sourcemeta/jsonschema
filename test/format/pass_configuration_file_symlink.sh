#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/config.json"
{
  "baseUri": "https://example.com",
  "title": "My project"
}
EOF

ln -s config.json "$TMP/jsonschema.json"

cd "$TMP"
"$1" fmt jsonschema.json > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Interpreting as a configuration file: $(realpath "$TMP")/config.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cat << 'EOF' > "$TMP/expected_file.txt"
{
  "title": "My project",
  "baseUri": "https://example.com"
}
EOF

diff "$TMP/config.json" "$TMP/expected_file.txt"
