#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema",
  "type": "object",
  "properties": {
    "name": {
      "type": "string"
    }
  }
}
EOF

mkdir -p "$TMP/instances"
mkdir -p "$TMP/instances/drafts"
mkdir -p "$TMP/instances/experimental"

cat << 'EOF' > "$TMP/instances/valid.json"
{ "name": "Alice" }
EOF

cat << 'EOF' > "$TMP/instances/drafts/invalid_config.json"
{ "name": 123 }
EOF

cat << 'EOF' > "$TMP/instances/experimental/invalid_cli.json"
{ "name": 456 }
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "ignore": [
    "./instances/drafts"
  ]
}
EOF

"$1" validate "$TMP/schema.json" "$TMP/instances" --ignore "$TMP/instances/experimental" --verbose 2> "$TMP/output.txt"

cat << EOF > "$TMP/expected.txt"
Ignoring path: "$(realpath "$TMP")/instances/experimental"
Ignoring path from configuration: "$(realpath "$TMP")/instances/drafts"
ok: $(realpath "$TMP")/instances/valid.json
  matches $(realpath "$TMP")/schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
