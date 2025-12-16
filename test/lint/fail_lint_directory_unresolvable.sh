#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/foo.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema"
}
EOF

cat << 'EOF' > "$TMP/bar.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema"
}
EOF

cat << 'EOF' > "$TMP/baz.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema",
  "allOf": [ { "$ref": "https://example.com/unknown" } ],
  "default": 1
}
EOF

"$1" lint "$TMP" --verbose >"$TMP/output.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
Linting: $(realpath "$TMP")/bar.json
Linting: $(realpath "$TMP")/baz.json
error: Could not resolve the reference to an external schema
  at identifier https://example.com/unknown
  at file path $(realpath "$TMP")/baz.json

This is likely because you forgot to import such schema using \`--resolve/-r\`
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
