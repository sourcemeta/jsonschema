Sourcemeta Extension Vocabulary
===============================

1. Introduction
---------------

The Sourcemeta Extension vocabulary, identified by the URI
`tag:sourcemeta.com,2026:extension/v1`, defines JSON Schema keywords that
annotate schemas with evaluation behavior and with the information required to
promote valid instances to JSON-LD [JSONLD11], the JSON-based serialization of
RDF [RDF11]. Every keyword in this vocabulary is an annotation. No keyword
directly asserts constraints on the instance, though `x-format-assertion`
changes how a sibling keyword is evaluated.

2. Versioning
-------------

The vocabulary URI identifies major version 1 of this vocabulary. Backwards
compatible changes, such as the introduction of new keywords, MAY be made to
this vocabulary without changing its URI. A breaking change MUST result in a
new vocabulary published under a new major version with a new URI.

3. Keywords
-----------

| Keyword | Value | Applies To |
|---------|-------|------------|
| [`x-format-assertion`](#31-x-format-assertion) | A boolean | Any subschema |
| [`x-jsonld-id`](#321-x-jsonld-id) | An absolute IRI | Property subschemas |
| [`x-jsonld-type`](#322-x-jsonld-type) | An absolute IRI or array of absolute IRIs | Object or reference subschemas |
| [`x-jsonld-reverse`](#323-x-jsonld-reverse) | An absolute IRI | Property subschemas |
| [`x-jsonld-datatype`](#324-x-jsonld-datatype) | An absolute IRI | Scalar subschemas |
| [`x-jsonld-language`](#325-x-jsonld-language) | A language tag | String subschemas |
| [`x-jsonld-direction`](#326-x-jsonld-direction) | `ltr` or `rtl` | String subschemas |
| [`x-jsonld-json`](#327-x-jsonld-json) | A boolean | Any subschema |
| [`x-jsonld-graph`](#328-x-jsonld-graph) | A boolean | Object subschemas |
| [`x-jsonld-container`](#329-x-jsonld-container) | `@list`, `@set`, `@language`, or `@index` | Array or object property subschemas |
| [`x-jsonld-self`](#3210-x-jsonld-self) | A URI Template | Scalar or object subschemas |

### 3.1. `x-format-assertion`

The value of this keyword MUST be a boolean.

When the value is `true`, a `format` keyword in the same schema object MUST be
evaluated as an assertion rather than as an annotation. When the value is
`false` or the keyword is absent, the evaluation of `format` is unchanged.
This keyword has no effect on any other keyword.

### 3.2. JSON-LD

The keywords in this section map the instance locations that their subschemas
successfully evaluate to nodes, literals, and edges of a JSON-LD document. A
keyword takes effect at every instance location its subschema evaluates. When
several subschemas annotate the same instance location, their annotations are
merged, and a keyword that takes a single value MUST NOT be assigned two
different values for the same location.

#### 3.2.1. `x-jsonld-id`

The value of this keyword MUST be a string representing an absolute IRI
[RFC3987].

This keyword declares the predicate IRI that connects the annotated instance
location, as the object, to its enclosing node. It MUST NOT be applied to the
document root, to an array element, or to a member of a location annotated
with `x-jsonld-container`, as such locations have no enclosing node to attach
a predicate to.

#### 3.2.2. `x-jsonld-type`

The value of this keyword MUST be a string representing an absolute IRI
[RFC3987], or an array of such strings.

This keyword declares the `@type` of the node that the annotated instance
location materializes as. Duplicate entries are collapsed, and an empty array
declares no types. It MUST be applied to a location whose value is an object,
unless the location is promoted to an identified node with `x-jsonld-self`.

#### 3.2.3. `x-jsonld-reverse`

The value of this keyword MUST be a string representing an absolute IRI
[RFC3987].

This keyword declares a reverse predicate IRI, asserting the edge with the
annotated instance location as the subject and the enclosing node as the
object, emitted through the JSON-LD `@reverse` keyword. The annotated location
MUST materialize as a node or as an array of nodes, as a literal cannot be the
subject of an edge. The placement requirements of `x-jsonld-id` also apply to
this keyword.

#### 3.2.4. `x-jsonld-datatype`

The value of this keyword MUST be a string representing an absolute IRI
[RFC3987].

This keyword declares the datatype IRI of the typed literal that the annotated
instance location materializes as, such as
`http://www.w3.org/2001/XMLSchema#date`. It MUST be applied to a location
whose value is a scalar, and it MUST NOT be combined with `x-jsonld-language`
or `x-jsonld-direction` at the same location.

#### 3.2.5. `x-jsonld-language`

The value of this keyword MUST be a string representing a well-formed language
tag [RFC5646].

This keyword declares the language of the language-tagged literal that the
annotated instance location materializes as. It MUST be applied to a location
whose value is a string.

#### 3.2.6. `x-jsonld-direction`

The value of this keyword MUST be the string `ltr` or the string `rtl`.

This keyword declares the base direction of the internationalised literal that
the annotated instance location materializes as. It MUST be applied to a
location whose value is a string.

#### 3.2.7. `x-jsonld-json`

The value of this keyword MUST be a boolean.

When the value is `true`, the annotated instance location is preserved
verbatim as an opaque JSON literal through the JSON-LD `@json` datatype, and
the keyword MUST NOT be combined with any keyword of this vocabulary other
than `x-jsonld-id` at the same location. When the value is `false`, the
keyword has no effect.

#### 3.2.8. `x-jsonld-graph`

The value of this keyword MUST be a boolean.

When the value is `true`, the edges of the node that the annotated instance
location materializes as are asserted inside a named `@graph` that the node
identifier denotes. The location value MUST be an object. When the value is
`false`, the keyword has no effect.

#### 3.2.9. `x-jsonld-container`

The value of this keyword MUST be the string `@list`, `@set`, `@language`, or
`@index`.

This keyword declares the container semantics of the annotated instance
location. The `@list` and `@set` containers range over an array value,
asserting an ordered RDF list and an unordered set respectively. The
`@language` container ranges over an object value, asserting language-tagged
literals. Its keys MUST be well-formed language tags [RFC5646], except for the
reserved key `@none` which asserts a literal with no language, and its members
MUST be strings, arrays of strings, or `null`, where `null` entries are
ignored. The `@index` container ranges over an object value whose keys carry
no RDF meaning. This keyword MUST NOT be combined with any keyword of this
vocabulary other than `x-jsonld-id` at the same location, and the members of a
`@language` container MUST NOT carry annotations of this vocabulary.

#### 3.2.10. `x-jsonld-self`

The value of this keyword MUST be a string representing a URI Template
[RFC6570].

This keyword mints the identifier of the node that the annotated instance
location materializes as, giving an object its `@id` or promoting a scalar to
an identified node. An object binds each template variable to the member of
that name, and a scalar binds the reserved variable `this` to its own value.
Every binding MUST be a non-empty string, a number, or a boolean, and the
expanded template MUST be an absolute IRI [RFC3987]. The location value MUST
NOT be an array, and the keyword MUST NOT be combined with
`x-jsonld-datatype`, `x-jsonld-language`, or `x-jsonld-direction` at the same
location.

4. References
-------------

### 4.1. Normative

- [RFC3987] Duerst, M. and M. Suignard, ["Internationalized Resource
  Identifiers (IRIs)"](https://www.rfc-editor.org/info/rfc3987), RFC 3987,
  January 2005
- [RFC5646] Phillips, A. and M. Davis, ["Tags for Identifying
  Languages"](https://www.rfc-editor.org/info/rfc5646), BCP 47, RFC 5646,
  September 2009
- [RFC6570] Gregorio, J., Fielding, R., Hadley, M., Nottingham, M., and D.
  Orchard, ["URI Template"](https://www.rfc-editor.org/info/rfc6570), RFC
  6570, March 2012
- [JSONLD11] Sporny, M., Longley, D., Kellogg, G., Lanthaler, M., Champin,
  P., and N. Lindström, ["JSON-LD 1.1"](https://www.w3.org/TR/json-ld11/),
  W3C Recommendation, July 2020

### 4.2. Informative

- [RDF11] Cyganiak, R., Wood, D., and M. Lanthaler, ["RDF 1.1 Concepts and
  Abstract Syntax"](https://www.w3.org/TR/rdf11-concepts/), W3C
  Recommendation, February 2014
