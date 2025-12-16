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
  "title": "Test",
  "description": "Test schema",
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
@- (annotation) "/description" (AnnotationEmit)
   value "Test schema"
   at instance location "" (line 1, column 1)
   at keyword location "https://example.com#/description"
   at vocabulary "https://json-schema.org/draft/2020-12/vocab/meta-data"

@- (annotation) "/title" (AnnotationEmit)
   value "Test"
   at instance location "" (line 1, column 1)
   at keyword location "https://example.com#/title"
   at vocabulary "https://json-schema.org/draft/2020-12/vocab/meta-data"

-> (push) "/properties" (LogicalWhenType)
   at instance location "" (line 1, column 1)
   at keyword location "https://example.com#/properties"
   at vocabulary "https://json-schema.org/draft/2020-12/vocab/applicator"

-> (push) "/properties/foo/type" (AssertionTypeStrict)
   at instance location "/foo" (line 1, column 3)
   at keyword location "https://example.com#/properties/foo/type"
   at vocabulary "https://json-schema.org/draft/2020-12/vocab/validation"

<- (pass) "/properties/foo/type" (AssertionTypeStrict)
   at instance location "/foo" (line 1, column 3)
   at keyword location "https://example.com#/properties/foo/type"
   at vocabulary "https://json-schema.org/draft/2020-12/vocab/validation"

@- (annotation) "/properties" (AnnotationEmit)
   value "foo"
   at instance location "" (line 1, column 1)
   at keyword location "https://example.com#/properties"
   at vocabulary "https://json-schema.org/draft/2020-12/vocab/applicator"

<- (pass) "/properties" (LogicalWhenType)
   at instance location "" (line 1, column 1)
   at keyword location "https://example.com#/properties"
   at vocabulary "https://json-schema.org/draft/2020-12/vocab/applicator"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
