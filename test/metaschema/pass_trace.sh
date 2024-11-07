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
<- (pass) "/dependencies"
   at ""
-> (push) "/properties"
   at ""
-> (push) "/properties/$schema/type"
   at "/$schema"
<- (pass) "/properties/$schema/type"
   at "/$schema"
<- (pass) "/properties"
   at ""
-> (push) "/type"
   at ""
<- (pass) "/type"
   at ""
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
