#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "title": "Test",
  "description": "Test schema",
  "type": "string",
  "examples": [ "foo" ],
  "$schema": "http://json-schema.org/draft-06/schema#",
  "const": "foo"
}
EOF

"$1" lint "$TMP/schema.json" --fix --format > "$TMP/output.txt" 2>&1

cat << 'EOF' > "$TMP/expected_output.txt"
.
EOF
diff "$TMP/output.txt" "$TMP/expected_output.txt"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "http://json-schema.org/draft-06/schema#",
  "title": "Test",
  "description": "Test schema",
  "examples": [ "foo" ],
  "const": "foo"
}
EOF

diff "$TMP/schema.json" "$TMP/expected.json"
