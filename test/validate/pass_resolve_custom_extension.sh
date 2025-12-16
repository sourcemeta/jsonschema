#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "id": "https://example.com",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema",
  "properties": {
    "foo": { "$ref": "foo" },
    "bar": { "$ref": "bar" }
  }
}
EOF

cat << 'EOF' > "$TMP/foo.schema"
{
  "id": "https://example.com/foo",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "integer"
}
EOF

mkdir "$TMP/schemas"
cat << 'EOF' > "$TMP/schemas/baz.schema"
{
  "id": "https://example.com/baz",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "array"
}
EOF

cat << 'EOF' > "$TMP/schemas/ignore-me.txt"
Foo Bar
EOF

mkdir "$TMP/schemas/nested"
cat << 'EOF' > "$TMP/schemas/nested/bar.schema"
{
  "id": "https://example.com/bar",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "foo": 1 }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --resolve "$TMP/foo.schema" --resolve "$TMP/schemas" --extension .schema
