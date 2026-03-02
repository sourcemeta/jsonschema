#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object"
}
EOF

# Generate a large JSON object with 1000 keys using POSIX awk
awk 'BEGIN {
  printf "{"
  for (i = 0; i < 1000; i++) {
    if (i > 0) printf ","
    printf "\"key_%d\":%d", i, i
  }
  printf "}"
}' | "$1" validate "$TMP/schema.json" - > "$TMP/output.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "0" || exit 1
