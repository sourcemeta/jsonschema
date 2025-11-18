#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "object",
  "properties": {
    "name": {
      "type": "string"
    },
    "age": {
      "type": "integer"
    }
  }
}
EOF

mkdir "$TMP/instances"

cat << 'EOF' > "$TMP/instances/instance_1.json"
{ "name": "Alice", "age": 30 }
EOF

cat << 'EOF' > "$TMP/instances/instance_2.json"
{ "name": "Bob", "age": 25 }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instances" --fast --json > "$TMP/stdout.txt" 2> "$TMP/stderr.txt"

cat << EOF > "$TMP/expected_stderr.txt"
$(realpath "$TMP")/instances/instance_1.json
$(realpath "$TMP")/instances/instance_2.json
EOF

cat << 'EOF' > "$TMP/expected_stdout.txt"
{
  "valid": true
}
{
  "valid": true
}
EOF

diff "$TMP/stderr.txt" "$TMP/expected_stderr.txt"
diff "$TMP/stdout.txt" "$TMP/expected_stdout.txt"
