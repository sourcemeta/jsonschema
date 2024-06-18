#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

"$1" validate 2>"$TMP/stderr.txt" \
  && CODE="$?" || CODE="$?"

if [ "$CODE" = "0" ]
then
  echo "FAIL" 1>&2
  exit 1
fi

cat << 'EOF' > "$TMP/expected.txt"
error: This command expects to pass a path to a schema and a
path to an instance to validate against the schema. For example:

  jsonschema validate path/to/schema.json path/to/instance.json
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
echo "PASS" 1>&2
