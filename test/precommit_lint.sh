#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

git -C "$TMP" init

cat << EOF > "$TMP/.pre-commit-config.yaml"
repos:
- repo: $(dirname "$(dirname "$(pwd)")")
  rev: HEAD
  hooks:
  - id: sourcemeta-jsonschema-lint
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "string",
  "enum": [ "foo" ]
}
EOF

git -C "$TMP" add .

cd "$TMP"
pre-commit install
pre-commit run --files schema.json > "$TMP/out.txt" 2>&1 && CODE="$?" || CODE="$?"
cat "$TMP/out.txt"
test "$CODE" = "1" || exit 1
grep -q "enum_with_type" "$TMP/out.txt" || exit 1
