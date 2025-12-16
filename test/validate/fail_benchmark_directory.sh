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

mkdir "$TMP/instances"

cat << 'EOF' > "$TMP/instances/instance_1.json"
"foo"
EOF

cat << 'EOF' > "$TMP/instances/instance_2.json"
"bar"
EOF

"$1" validate "$TMP/schema.json" "$TMP/instances" --benchmark 2> "$TMP/stderr.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
error: The `--benchmark/-b` option is only allowed given a single instance
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" validate "$TMP/schema.json" "$TMP/instances" --benchmark --json > "$TMP/stdout.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
{
  "error": "The `--benchmark/-b` option is only allowed given a single instance"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
