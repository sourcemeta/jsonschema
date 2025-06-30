#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "properties": {
    "foo": {
      "type": "string"
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "foo": "bar" }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" --trace > "$TMP/output.txt"

cat << EOF > "$TMP/expected.txt"
-> (push) "/properties" (LogicalAnd)
   at ""
   at keyword location "file://$(realpath "$TMP")/schema.json#/properties"
   at vocabulary "https://json-schema.org/draft/2020-12/vocab/applicator"

-> (push) "/properties/foo/type" (AssertionTypeStrict)
   at "/foo"
   at keyword location "file://$(realpath "$TMP")/schema.json#/properties/foo/type"
   at vocabulary "https://json-schema.org/draft/2020-12/vocab/validation"

<- (pass) "/properties/foo/type" (AssertionTypeStrict)
   at "/foo"
   at keyword location "file://$(realpath "$TMP")/schema.json#/properties/foo/type"
   at vocabulary "https://json-schema.org/draft/2020-12/vocab/validation"

@- (annotation) "/properties" (AnnotationEmit)
   value "foo"
   at ""
   at keyword location "file://$(realpath "$TMP")/schema.json#/properties"
   at vocabulary "https://json-schema.org/draft/2020-12/vocab/applicator"

<- (pass) "/properties" (LogicalAnd)
   at ""
   at keyword location "file://$(realpath "$TMP")/schema.json#/properties"
   at vocabulary "https://json-schema.org/draft/2020-12/vocab/applicator"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
