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

"$1" validate "$TMP/schema.json" "$TMP/instances" --fast 2> "$TMP/stderr.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
fail: $(realpath "$TMP")/instances/instance_2.json
error: Schema validation failure
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
