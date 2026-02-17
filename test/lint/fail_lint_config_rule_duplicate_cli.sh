#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/config_rule.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "my_rule",
  "description": "Config rule",
  "required": [ "type" ]
}
EOF

cat << 'EOF' > "$TMP/cli_rule.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "my_rule",
  "description": "CLI rule",
  "required": [ "description" ]
}
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "lint": {
    "rules": [
      "./config_rule.json"
    ]
  }
}
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

cd "$TMP"
"$1" lint --rule "$TMP/cli_rule.json" "$TMP/schema.json" \
  > "$TMP/output.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: A lint rule with this name already exists
  at file path $(realpath "$TMP")/cli_rule.json
  at rule my_rule
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cd "$TMP"
"$1" lint --rule "$TMP/cli_rule.json" --json "$TMP/schema.json" \
  > "$TMP/output_json.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected_json.txt"
{
  "error": "A lint rule with this name already exists",
  "filePath": "$(realpath "$TMP")/cli_rule.json",
  "rule": "my_rule"
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"
