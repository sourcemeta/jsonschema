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

mkdir "$TMP/instances"

cat << 'EOF' > "$TMP/instances/instance_1.json"
{ "name": "Alice" }
EOF

cat << 'EOF' > "$TMP/instances/instance_2.json"
{ "name": 123 }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instances" --fast --json > "$TMP/stdout.txt" 2> "$TMP/stderr.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Validation failure
test "$EXIT_CODE" = "2" || exit 1

cat << EOF > "$TMP/expected_stderr.txt"
$(realpath "$TMP")/instances/instance_1.json
$(realpath "$TMP")/instances/instance_2.json
EOF

cat << 'EOF' > "$TMP/expected_stdout.txt"
{
  "valid": true
}
{
  "valid": false
}
EOF

diff "$TMP/stderr.txt" "$TMP/expected_stderr.txt"
diff "$TMP/stdout.txt" "$TMP/expected_stdout.txt"
