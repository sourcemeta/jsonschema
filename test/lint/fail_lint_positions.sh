#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "additionalProperties": {
    "unknown-1": 1,
    "unknown-2": 2,
    "unknown-3": 3
  }
}
EOF

cd "$TMP"
"$1" lint "$TMP/schema.json" >"$TMP/stderr.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
schema.json:4:5:
  Future versions of JSON Schema will refuse to evaluate unknown keywords or custom keywords from optional vocabularies that don't have an x- prefix (unknown_keywords_prefix)
    at location "/additionalProperties/unknown-1"
schema.json:5:5:
  Future versions of JSON Schema will refuse to evaluate unknown keywords or custom keywords from optional vocabularies that don't have an x- prefix (unknown_keywords_prefix)
    at location "/additionalProperties/unknown-2"
schema.json:6:5:
  Future versions of JSON Schema will refuse to evaluate unknown keywords or custom keywords from optional vocabularies that don't have an x- prefix (unknown_keywords_prefix)
    at location "/additionalProperties/unknown-3"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
