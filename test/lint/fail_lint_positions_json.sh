#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema",
  "additionalProperties": {
    "unknown-1": 1,
    "unknown-2": 2,
    "unknown-3": 3
  }
}
EOF

"$1" lint "$TMP/schema.json" --json >"$TMP/output.json" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Lint violation
test "$EXIT_CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.json"
{
  "valid": false,
  "health": 50,
  "errors": [
    {
      "path": "$(realpath "$TMP")/schema.json",
      "id": "unknown_keywords_prefix",
      "message": "Future versions of JSON Schema will refuse to evaluate unknown keywords or custom keywords from optional vocabularies that don't have an x- prefix",
      "description": null,
      "schemaLocation": "/additionalProperties/unknown-1",
      "position": [ 6, 5, 6, 18 ]
    },
    {
      "path": "$(realpath "$TMP")/schema.json",
      "id": "unknown_keywords_prefix",
      "message": "Future versions of JSON Schema will refuse to evaluate unknown keywords or custom keywords from optional vocabularies that don't have an x- prefix",
      "description": null,
      "schemaLocation": "/additionalProperties/unknown-2",
      "position": [ 7, 5, 7, 18 ]
    },
    {
      "path": "$(realpath "$TMP")/schema.json",
      "id": "unknown_keywords_prefix",
      "message": "Future versions of JSON Schema will refuse to evaluate unknown keywords or custom keywords from optional vocabularies that don't have an x- prefix",
      "description": null,
      "schemaLocation": "/additionalProperties/unknown-3",
      "position": [ 8, 5, 8, 18 ]
    }
  ]
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
