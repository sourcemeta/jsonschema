#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$id": "https://example.com",
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

"$1" compile "$TMP/schema.json" > "$TMP/template.json"
"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --trace --template "$TMP/template.json" > "$TMP/output.txt"

cat << EOF > "$TMP/expected.txt"
-> (push) "/properties" (LogicalWhenType)
   at ""
   at keyword location "https://example.com#/properties"
   at vocabulary "https://json-schema.org/draft/2020-12/vocab/applicator"

-> (push) "/properties/foo/type" (AssertionTypeStrict)
   at "/foo"
   at keyword location "https://example.com#/properties/foo/type"
   at vocabulary "https://json-schema.org/draft/2020-12/vocab/validation"

<- (pass) "/properties/foo/type" (AssertionTypeStrict)
   at "/foo"
   at keyword location "https://example.com#/properties/foo/type"
   at vocabulary "https://json-schema.org/draft/2020-12/vocab/validation"

@- (annotation) "/properties" (AnnotationEmit)
   value "foo"
   at ""
   at keyword location "https://example.com#/properties"
   at vocabulary "https://json-schema.org/draft/2020-12/vocab/applicator"

<- (pass) "/properties" (LogicalWhenType)
   at ""
   at keyword location "https://example.com#/properties"
   at vocabulary "https://json-schema.org/draft/2020-12/vocab/applicator"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
