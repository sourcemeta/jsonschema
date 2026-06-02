#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/rule.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "title_must_be_email",
  "description": "The schema title must be a valid email address",
  "type": "object",
  "properties": {
    "title": { "type": "string", "format": "email" }
  },
  "required": [ "title" ]
}
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "not-an-email",
  "type": "string"
}
EOF

cd "$TMP"
"$1" lint --rule "$TMP/rule.json" --only title_must_be_email \
  --format-assertion "$TMP/schema.json" \
  > "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Lint violation
test "$EXIT_CODE" = "2"

cat << 'EOF' > "$TMP/expected.txt"
schema.json:3:3:
  The schema title must be a valid email address (title_must_be_email)
    at location "/title"
    The string value "not-an-email" was expected to represent a valid email address
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
