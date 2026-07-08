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
  "properties": {
    "foo": {
      "type": "string"
    }
  }
}
EOF

cat << 'EOF' | gzip > "$TMP/instance.jsonl.gz"
{ "foo": "first" }
{ "foo": "second" }
{ "foo": "third" }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.jsonl.gz" --verbose 2> "$TMP/output.txt" 1>&2

cat << EOF > "$TMP/expected.txt"
Interpreting input as GZIP-compressed JSONL: $(realpath "$TMP")/instance.jsonl.gz
ok: $(realpath "$TMP")/instance.jsonl.gz (entry #1)
  matches $(realpath "$TMP")/schema.json
annotation: "Test schema"
  at instance location ""
  at evaluate path "/description"
annotation: "Test"
  at instance location ""
  at evaluate path "/title"
annotation: "foo"
  at instance location ""
  at evaluate path "/properties"
ok: $(realpath "$TMP")/instance.jsonl.gz (entry #2)
  matches $(realpath "$TMP")/schema.json
annotation: "Test schema"
  at instance location ""
  at evaluate path "/description"
annotation: "Test"
  at instance location ""
  at evaluate path "/title"
annotation: "foo"
  at instance location ""
  at evaluate path "/properties"
ok: $(realpath "$TMP")/instance.jsonl.gz (entry #3)
  matches $(realpath "$TMP")/schema.json
annotation: "Test schema"
  at instance location ""
  at evaluate path "/description"
annotation: "Test"
  at instance location ""
  at evaluate path "/title"
annotation: "foo"
  at instance location ""
  at evaluate path "/properties"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
