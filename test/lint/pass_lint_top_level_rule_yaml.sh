#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/rule.yaml"
$schema: "https://json-schema.org/draft/2020-12/schema"
title: require_id
description: The root schema must declare an $id
required:
  - "$id"
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/schema",
  "type": "object",
  "properties": {
    "foo": { "type": "string" }
  }
}
EOF

"$1" lint --top-level-rule "$TMP/rule.yaml" --only require_id "$TMP/schema.json" \
  > "$TMP/output.txt" 2>&1

cat << 'EOF' > "$TMP/expected.txt"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
