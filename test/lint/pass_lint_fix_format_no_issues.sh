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
  "examples": [ "foo" ],
  "type": "string",
  "$schema": "http://json-schema.org/draft-06/schema#"
}
EOF

# First verify there are no lint issues
"$1" lint "$TMP/schema.json" > "$TMP/output.txt" 2>&1

cat << 'EOF' > "$TMP/expected_output.txt"
EOF
diff "$TMP/output.txt" "$TMP/expected_output.txt"

# Now test that --fix --format still formats the file
"$1" lint "$TMP/schema.json" --fix --format > "$TMP/output.txt" 2>&1

cat << 'EOF' > "$TMP/expected_output.txt"
EOF
diff "$TMP/output.txt" "$TMP/expected_output.txt"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "http://json-schema.org/draft-06/schema#",
  "title": "Test",
  "description": "Test schema",
  "examples": [ "foo" ],
  "type": "string"
}
EOF

diff "$TMP/schema.json" "$TMP/expected.json"
