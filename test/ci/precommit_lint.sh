#!/bin/sh

set -o errexit
set -o nounset

# Ensure pipx-installed binaries are in PATH
export PATH="$HOME/.local/bin:$PATH"

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

ROOT="$(dirname "$(dirname "$(pwd)")")"

cat << EOF > "$TMP/.pre-commit-config.yaml"
repos:
- repo: $ROOT
  rev: HEAD
  hooks:
  - id: sourcemeta-jsonschema-lint
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema",
  "type": "string",
  "enum": [ "foo" ]
}
EOF

git -C "$TMP" init
git -C "$TMP" add .

cd "$TMP"
pre-commit install
PATH="$ROOT/bin:$PATH" \
  pre-commit run --files schema.json > "$TMP/out.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
cat "$TMP/out.txt"
test "$EXIT_CODE" = "1" || exit 1
grep -q "enum_with_type" "$TMP/out.txt" || exit 1
