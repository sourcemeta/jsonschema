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
  "title": "Test",
  "description": "Test schema",
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

"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --resolve "$TMP/remote.json" --verbose > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
ok: $(realpath "$TMP")/instance.json
  matches $(realpath "$TMP")/schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
