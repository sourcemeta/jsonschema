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

cd "$TMP"
pre-commit install
pre-commit run --all-files && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1
