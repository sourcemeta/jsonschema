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

-> (push) "/properties/minimum/type"
   at "/minimum"
   at keyword location "http://json-schema.org/draft-04/schema#/properties/minimum/type"

<- (fail) "/properties/minimum/type"
   at "/minimum"
   at keyword location "http://json-schema.org/draft-04/schema#/properties/minimum/type"

<- (fail) "/properties"
   at ""
   at keyword location "http://json-schema.org/draft-04/schema#/properties"
EOF

cat << 'EOF' > "$TMP/expected-2.txt"
-> (push) "/dependencies"
   at ""
   at keyword location "http://json-schema.org/draft-04/schema#/dependencies"

<- (pass) "/dependencies"
   at ""
   at keyword location "http://json-schema.org/draft-04/schema#/dependencies"

-> (push) "/properties"
   at ""
   at keyword location "http://json-schema.org/draft-04/schema#/properties"

-> (push) "/properties/minimum/type"
   at "/minimum"
   at keyword location "http://json-schema.org/draft-04/schema#/properties/minimum/type"

<- (fail) "/properties/minimum/type"
   at "/minimum"
   at keyword location "http://json-schema.org/draft-04/schema#/properties/minimum/type"

<- (fail) "/properties"
   at ""
   at keyword location "http://json-schema.org/draft-04/schema#/properties"
EOF

diff "$TMP/output.txt" "$TMP/expected-1.txt" || \
  diff "$TMP/output.txt" "$TMP/expected-2.txt"
