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

<- (pass) "/properties" (LoopPropertiesMatch)
   at ""
   at keyword location "http://json-schema.org/draft-04/schema#/properties"
   at vocabulary "http://json-schema.org/draft-04/schema#"

-> (push) "/type" (AssertionTypeStrict)
   at ""
   at keyword location "http://json-schema.org/draft-04/schema#/type"
   at vocabulary "http://json-schema.org/draft-04/schema#"

<- (pass) "/type" (AssertionTypeStrict)
   at ""
   at keyword location "http://json-schema.org/draft-04/schema#/type"
   at vocabulary "http://json-schema.org/draft-04/schema#"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
