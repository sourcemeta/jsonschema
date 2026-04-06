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

cat << 'EOF' > "$TMP/instance.json"
{ "name": "Alice" }
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "defaultDialect": "http://json-schema.org/draft-04/schema#",
  "ignore": [
    "./jsonschema.json"
  ]
}
EOF

BIN="$(realpath "$1")"
cd "$TMP"
"$BIN" validate schema.json --verbose > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
warning: Recursively processing every file in $(realpath "$TMP") as the configuration file does not set an explicit path
Ignoring path from configuration: "$(realpath "$TMP")/jsonschema.json"
Using extension: .json
Using extension: .yaml
Using extension: .yml
ok: $(realpath "$TMP")/instance.json
  matches $(realpath "$TMP")/schema.json
ok: $(realpath "$TMP")/schema.json
  matches $(realpath "$TMP")/schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
