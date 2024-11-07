#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{ "$schema": "http://json-schema.org/draft-04/schema#" }
EOF

"$1" metaschema "$TMP/schema.json" --trace > "$TMP/output.txt"

cat << 'EOF' > "$TMP/expected.txt"
-> (push) "/dependencies"
   at ""
   at keyword location "http://json-schema.org/draft-04/schema#/dependencies"

<- (pass) "/dependencies"
   at ""
   at keyword location "http://json-schema.org/draft-04/schema#/dependencies"

-> (push) "/properties"
   at ""
   at keyword location "http://json-schema.org/draft-04/schema#/properties"

-> (push) "/properties/$schema/type"
   at "/$schema"
   at keyword location "http://json-schema.org/draft-04/schema#/properties/$schema/type"

<- (pass) "/properties/$schema/type"
   at "/$schema"
   at keyword location "http://json-schema.org/draft-04/schema#/properties/$schema/type"

<- (pass) "/properties"
   at ""
   at keyword location "http://json-schema.org/draft-04/schema#/properties"

-> (push) "/type"
   at ""
   at keyword location "http://json-schema.org/draft-04/schema#/type"

<- (pass) "/type"
   at ""
   at keyword location "http://json-schema.org/draft-04/schema#/type"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
