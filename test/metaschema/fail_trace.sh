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
test "$CODE" = "2" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
-> (push) "/dependencies" (AssertionPropertyDependencies)
   at ""
   at keyword location "http://json-schema.org/draft-04/schema#/dependencies"
   at vocabulary "http://json-schema.org/draft-04/schema#"

<- (pass) "/dependencies" (AssertionPropertyDependencies)
   at ""
   at keyword location "http://json-schema.org/draft-04/schema#/dependencies"
   at vocabulary "http://json-schema.org/draft-04/schema#"

-> (push) "/properties" (LoopPropertiesMatch)
   at ""
   at keyword location "http://json-schema.org/draft-04/schema#/properties"
   at vocabulary "http://json-schema.org/draft-04/schema#"

-> (push) "/properties/$schema/type" (AssertionTypeStrict)
   at "/$schema"
   at keyword location "http://json-schema.org/draft-04/schema#/properties/$schema/type"
   at vocabulary "http://json-schema.org/draft-04/schema#"

<- (pass) "/properties/$schema/type" (AssertionTypeStrict)
   at "/$schema"
   at keyword location "http://json-schema.org/draft-04/schema#/properties/$schema/type"
   at vocabulary "http://json-schema.org/draft-04/schema#"

-> (push) "/properties/minimum/type" (AssertionTypeStrictAny)
   at "/minimum"
   at keyword location "http://json-schema.org/draft-04/schema#/properties/minimum/type"
   at vocabulary "http://json-schema.org/draft-04/schema#"

<- (fail) "/properties/minimum/type" (AssertionTypeStrictAny)
   at "/minimum"
   at keyword location "http://json-schema.org/draft-04/schema#/properties/minimum/type"
   at vocabulary "http://json-schema.org/draft-04/schema#"

<- (fail) "/properties" (LoopPropertiesMatch)
   at ""
   at keyword location "http://json-schema.org/draft-04/schema#/properties"
   at vocabulary "http://json-schema.org/draft-04/schema#"
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

diff "$TMP/output.txt" "$TMP/expected.txt"
