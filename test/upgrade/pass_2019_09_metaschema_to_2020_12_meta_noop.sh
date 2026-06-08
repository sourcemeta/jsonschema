#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2019-09/schema",
  "$id": "https://example.com/my-metaschema",
  "$vocabulary": {
    "https://json-schema.org/draft/2019-09/vocab/core": true,
    "https://json-schema.org/draft/2019-09/vocab/applicator": true,
    "https://json-schema.org/draft/2019-09/vocab/validation": true,
    "https://json-schema.org/draft/2019-09/vocab/meta-data": true,
    "https://json-schema.org/draft/2019-09/vocab/format": false,
    "https://json-schema.org/draft/2019-09/vocab/content": true
  },
  "title": "My Custom Meta-Schema",
  "type": [ "object", "boolean" ]
}
EOF

"$1" upgrade --meta --to 2020-12 "$TMP/schema.json" > "$TMP/output_meta.json"
"$1" upgrade --to 2020-12 "$TMP/schema.json" > "$TMP/output_plain.json"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/my-metaschema",
  "$vocabulary": {
    "https://json-schema.org/draft/2020-12/vocab/core": true,
    "https://json-schema.org/draft/2020-12/vocab/applicator": true,
    "https://json-schema.org/draft/2020-12/vocab/unevaluated": true,
    "https://json-schema.org/draft/2020-12/vocab/validation": true,
    "https://json-schema.org/draft/2020-12/vocab/meta-data": true,
    "https://json-schema.org/draft/2020-12/vocab/format-annotation": false,
    "https://json-schema.org/draft/2020-12/vocab/content": true
  },
  "title": "My Custom Meta-Schema",
  "type": [ "object", "boolean" ]
}
EOF

diff "$TMP/output_meta.json" "$TMP/output_plain.json"
diff "$TMP/output_meta.json" "$TMP/expected.json"
