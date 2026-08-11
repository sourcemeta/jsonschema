#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/nested.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/instance.json"
"Hello World"
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$ref": "./nested.json"
}
EOF

cd "$TMP"

# A relative reference from standard input must fail loudly rather than
# quietly resolve against whatever sits in the working directory
"$1" validate - "$TMP/instance.json" < "$TMP/schema.json" 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4"

cat << 'EOF' > "$TMP/expected.txt"
error: Could not resolve the reference to an external schema
  at identifier tag:sourcemeta.com,2026:jsonschema/nested.json
  at file path tag:sourcemeta.com,2026:jsonschema/stdin

This is likely because you forgot to import such schema using `--resolve/-r`
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" validate - "$TMP/instance.json" --json < "$TMP/schema.json" \
  > "$TMP/stdout.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4"

cat << 'EOF' > "$TMP/expected.txt"
{
  "error": "Could not resolve the reference to an external schema",
  "identifier": "tag:sourcemeta.com,2026:jsonschema/nested.json",
  "filePath": "tag:sourcemeta.com,2026:jsonschema/stdin"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
