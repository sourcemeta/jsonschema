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
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "defaultDialect": "http://json-schema.org/draft-04/schema#",
  "ignore": [
    "./jsonschema.json"
  ]
}
EOF

cd "$TMP"
"$1" lint --verbose > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
warning: Recursively processing every file in $(realpath "$TMP") as the configuration file does not set an explicit path
Ignoring path from configuration: "$(realpath "$TMP")/jsonschema.json"
Using extension: .json
Using extension: .yaml
Using extension: .yml
Linting: $(realpath "$TMP")/schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
