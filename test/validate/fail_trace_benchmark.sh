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

cat << 'EOF' > "$TMP/instance.json"
"hello"
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" --trace --benchmark 2>"$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "5"

cat << 'EOF' > "$TMP/expected.txt"
error: The `--trace/-t` and `--benchmark/-b` options are mutually exclusive
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

"$1" validate "$TMP/schema.json" "$TMP/instance.json" --trace --benchmark --json > "$TMP/stdout_json.txt" 2>/dev/null \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "5"

cat << 'EOF' > "$TMP/expected_json.txt"
{
  "error": "The `--trace/-t` and `--benchmark/-b` options are mutually exclusive"
}
EOF

diff "$TMP/stdout_json.txt" "$TMP/expected_json.txt"
