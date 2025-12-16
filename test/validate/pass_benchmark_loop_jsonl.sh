#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test schema",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/instance.jsonl"
"Hello World!"
"Bonjour tout le monde!"
""
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.jsonl" --benchmark --loop 1000 > "$TMP/output.txt"

if ! grep -E "/instance\.jsonl\[[0-9]+]: PASS [0-9]+\.[0-9]+ \+- [0-9]+\.[0-9]+ us \([0-9]+\.[0-9]+\)$" "$TMP/output.txt" > /dev/null
then
  cat "$TMP/output.txt" >&2
  exit 1
fi
