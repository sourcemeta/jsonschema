#!/bin/sh
set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/old.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "properties": {
    "age": { "type": "integer" }
  }
}
EOF

cat << 'EOF' > "$TMP/new.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "properties": {
    "age": { "type": "integer" }
  },
  "required": ["age"]
}
EOF

"$1" compat "$TMP/old.json" "$TMP/new.json" --fail-on-breaking > "$TMP/stdout" 2> "$TMP/stderr" && EXIT_CODE="$?" || EXIT_CODE="$?"

if [ "$EXIT_CODE" != "2" ]
then
  echo "FAIL: compat --fail-on-breaking did not return exit code 2" 1>&2
  exit 1
fi

if [ -s "$TMP/stderr" ]
then
  echo "FAIL: Unexpected stderr output for compat --fail-on-breaking" 1>&2
  exit 1
fi

cat << 'EOF' > "$TMP/expected.txt"
Breaking Changes:

- property "age" is now required
EOF

diff "$TMP/stdout" "$TMP/expected.txt"

echo "PASS" 1>&2
