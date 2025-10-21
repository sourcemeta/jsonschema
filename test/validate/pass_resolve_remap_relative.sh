#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "id": "https://example.com",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "properties": {
    "foo": { "$ref": "other" }
  }
}
EOF

cat << 'EOF' > "$TMP/remote.json"
{
  "id": "https://example.com/nested",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "integer"
}
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "resolve": {
    "https://example.com/other": "https://example.com/middle",
    "https://example.com/middle": "https://example.com/nested"
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "foo": 1 }
EOF

cd "$TMP"

"$1" validate schema.json instance.json \
  --resolve remote.json --verbose > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Using configuration file: $(realpath "$TMP")/jsonschema.json
Detecting schema resources from file: $(realpath "$TMP")/remote.json
Importing schema into the resolution context: file://$(realpath "$TMP")/remote.json
Importing schema into the resolution context: https://example.com/nested
Resolving https://example.com/other as https://example.com/middle given the configuration file
Resolving https://example.com/middle as https://example.com/nested given the configuration file
ok: $(realpath "$TMP")/instance.json
  matches $(realpath "$TMP")/schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
