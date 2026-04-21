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

cd "$TMP"
"$1" lint "$TMP" --fix --verbose > "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Lint violation
test "$EXIT_CODE" = "2"

cat << EOF > "$TMP/expected.txt"
Linting: $(realpath "$TMP")/bar.json
Linting: $(realpath "$TMP")/baz.json
baz.json:5:16:
  External references must point to schemas that can be resolved (invalid_external_ref)
    at location "/allOf/0/\$ref"
Linting: $(realpath "$TMP")/foo.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
