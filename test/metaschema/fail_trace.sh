#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "minimum": "foo"
}
EOF

"$1" metaschema "$TMP/schema.json" --trace > "$TMP/output.txt" \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

# Order of execution can vary

cat << 'EOF' > "$TMP/expected-1.txt"
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
-> (push) "/properties/minimum/type"
   at "/minimum"
<- (fail) "/properties/minimum/type"
   at "/minimum"
<- (fail) "/properties"
   at ""
EOF

cat << 'EOF' > "$TMP/expected-2.txt"
-> (push) "/dependencies"
   at ""
<- (pass) "/dependencies"
   at ""
-> (push) "/properties"
   at ""
-> (push) "/properties/minimum/type"
   at "/minimum"
<- (fail) "/properties/minimum/type"
   at "/minimum"
<- (fail) "/properties"
   at ""
EOF

diff "$TMP/output.txt" "$TMP/expected-1.txt" || \
  diff "$TMP/output.txt" "$TMP/expected-2.txt"
