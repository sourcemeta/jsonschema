#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/instance_1.json"
"foo"
EOF

cat << 'EOF' > "$TMP/instance_2.json"
"bar"
EOF

echo '"world"' | "$1" validate "$TMP/schema.json" "$TMP/instance_1.json" - "$TMP/instance_2.json" --trace 2>"$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "5" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
error: The `--trace/-t` option is only allowed given a single instance
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
echo '"world"' | "$1" validate "$TMP/schema.json" "$TMP/instance_1.json" - "$TMP/instance_2.json" --trace --json >"$TMP/stdout.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "5" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
{
  "error": "The `--trace/-t` option is only allowed given a single instance"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
