#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema_1.json"
{
  "additionalProperties": false,
  "title": "Hello World",
  "properties": {"foo": {}, "bar": {}}
}
EOF

cat << 'EOF' > "$TMP/schema_2.json"
{"type": "string", "title": "My String"}
EOF

cd "$TMP"
"$1" fmt

cat << 'EOF' > "$TMP/expected_1.json"
{
  "title": "Hello World",
  "properties": {
    "bar": {},
    "foo": {}
  },
  "additionalProperties": false
}
EOF

cat << 'EOF' > "$TMP/expected_2.json"
{
  "title": "My String",
  "type": "string"
}
EOF

if ! cmp -s "$TMP/schema_1.json" "$TMP/expected_1.json"
then
  echo "GOT:" 1>&2
  cat "$TMP/schema_1.json" 1>&2
  echo "EXPECTED:" 1>&2
  cat "$TMP/expected_1.json" 1>&2
  echo "FAIL" 1>&2
  exit 1
fi

if ! cmp -s "$TMP/schema_2.json" "$TMP/expected_2.json"
then
  echo "GOT:" 1>&2
  cat "$TMP/schema_2.json" 1>&2
  echo "EXPECTED:" 1>&2
  cat "$TMP/expected_2.json" 1>&2
  echo "FAIL" 1>&2
  exit 1
fi
