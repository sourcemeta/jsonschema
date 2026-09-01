Linked Data (RDF)
=================

```sh
jsonschema rdf <schema.json|.yaml> <instance.json|.yaml>
  [--flatten/-l] [--compact/-c <context.json|.yaml>]
  [--fast/-f] [--format-assertion/-F]
  [--http/-h] [--verbose/-v] [--debug/-g]
  [--header/-H "<name>: <value>"]
  [--resolve/-r <schemas-or-directories> ...]
  [--extension/-e <extension>] [--ignore/-i <schemas-or-directories>]
  [--default-dialect/-d <uri>] [--json/-j]
```

> [!NOTE]
> See [Resolving External References](./guides/resolution.markdown) for every way of
> making referenced schemas available, including how to handle a reference whose
> URI differs from the identifier the target schema declares.

The same JSON Schema that validates your data can also declare what that data
means. The `rdf` command evaluates an instance against a schema annotated
with `x-jsonld-*` keywords and, if the instance is valid, prints the instance
promoted to [JSON-LD](https://www.w3.org/TR/json-ld11/), the JSON-based
serialization of [RDF](https://www.w3.org/RDF/). The result is a
knowledge-graph-ready document produced in a single evaluation pass, without
maintaining a separate JSON-LD `@context` that can silently drift from the
schema. To learn more about the motivation and design behind this feature,
refer to our [Fully solving JSON Schema and JSON-LD
interoperability](https://www.sourcemeta.com/blog/json-schema-jsonld-interoperability/)
blog post.

**This command is experimental and we are actively seeking feedback to make
this better. Please open an issue at
https://github.com/sourcemeta/jsonschema/issues with any problems you find.**

By default, the output is in [expanded
form](https://www.w3.org/TR/json-ld11/#expanded-document-form), which is
canonical and context-free. Pass `--flatten/-l` to
[flatten](https://www.w3.org/TR/json-ld11/#flattened-document-form) the
output, labelling every node, and `--compact/-c` with a context file to
[compact](https://www.w3.org/TR/json-ld11/#compacted-document-form) it into
idiomatic, human-friendly JSON-LD. Both options may be combined.

As with [`validate`](./validate.markdown), schemas are compiled in exhaustive
mode by default for better validation error messages. Pass `--fast/-f` to
optimise for speed at the expense of error message quality.

> [!NOTE]
> Annotation collection is a JSON Schema 2019-09 and 2020-12 feature, so this
> command requires the schema to have one of those dialects as its base
> dialect, and reports an error otherwise. Custom meta-schemas that build on
> those dialects work as expected. Referenced schemas may use older dialects
> and validate as usual, but their `x-jsonld-*` keywords do not emit
> annotations and are ignored. If your schemas use an older dialect, consider
> moving them to a newer one with the [`upgrade`](./upgrade.markdown)
> command.

The Annotation Vocabulary
-------------------------

The command resolves the following annotation keywords, which may be placed
on any subschema. Unknown `x-` prefixed keywords are standard-compliant JSON
Schema, so annotated schemas remain valid for every other tool.

| Keyword | Values | Applies To | Meaning |
|---------|--------|------------|---------|
| `x-jsonld-id` | An absolute IRI | property subschema | The predicate IRI the property maps to |
| `x-jsonld-type` | An absolute IRI or array of absolute IRIs | object or reference subschema | The node `@type` |
| `x-jsonld-reverse` | An absolute IRI | property subschema | A reverse predicate IRI, emitting `@reverse` edges |
| `x-jsonld-datatype` | An absolute IRI other than `rdf:langString` | scalar subschema | A typed literal datatype IRI, such as `http://www.w3.org/2001/XMLSchema#date`. Language-tagged literals are produced with `x-jsonld-language` instead |
| `x-jsonld-language` | A canonical [BCP 47](https://www.rfc-editor.org/info/bcp47) language tag | string subschema | The language of language-tagged literals |
| `x-jsonld-direction` | `ltr` or `rtl` | string subschema | The base direction for internationalised literals |
| `x-jsonld-json` | A boolean | any subschema | Treat the value as an opaque `@json` literal |
| `x-jsonld-graph` | A boolean | object subschema | Wrap the node's edges in a named `@graph` |
| `x-jsonld-container` | `@list`, `@set`, `@language`, or `@index` | array or object property subschema | The container semantics of the property |
| `x-jsonld-self` | An [RFC 6570](https://www.rfc-editor.org/rfc/rfc6570) URI template or a scheme identity name | scalar or object subschema | Mint the node `@id` from instance values, such as `https://www.iso.org/iso-4217/{this}` or `mailto` |
| `x-jsonld-override` | A boolean | any subschema | Give the schema object's own `x-jsonld-*` values precedence over conflicting ones from subschemas beneath it, such as a sibling `$ref` |
| `x-jsonld-value` | An absolute IRI | scalar subschema | Promote the scalar to a node that carries it as a literal under the declared predicate |
| `x-jsonld-constants` | An expanded-form node object fragment | object or promoted scalar subschema | Constant properties, fixed by the schema, merged into the node |

The guarantee this command makes is syntactic: if resolution succeeds, the
output is well-formed JSON-LD. Resolution errors enforce only what that
guarantee needs, such as keyword value grammar, annotation placement, and
single-value consistency, and every error cites the schema location of the
offending annotation. The command does not judge whether the IRIs, datatypes,
and language tags you declare make semantic sense. A schema can declare a
datatype whose lexical space its values never fit, or a misspelled ontology
term, and the command emits the annotations faithfully as written. That
correctness is on you as the schema author. In the future, we plan to provide
lint rules that catch more of these mistakes statically at design time, where
they belong, instead of paying for the checks on every promotion.

> [!NOTE]
> As a deliberate deviation from BCP 47's lenient approach to canonical identifiers
> (which JSON-LD 1.1 inherits), the language tags must be written in
> strictly canonical form in `x-jsonld-language` values as the map keys of an
> `@language` container in instance data and as `@language` members of
> `x-jsonld-constants` literals. For example, `en-US` is accepted while
> `en-us` is rejected (as a departure from BCP 47's lenient requirements).
>  This keeps language tag equality a plain cheap string comparison everywhere
> in the engine.

The variables of an `x-jsonld-self` URI template are matched verbatim against
instance property names, so a variable like `{+meta.slug}` binds a property
literally named `meta.slug`. There is no dotted path traversal into nested
objects.

Instead of a URI template, `x-jsonld-self` also accepts a scheme identity
name, which mints the canonical IRI that identifies the string value itself
in the named scheme. The engine owns the whole recipe, both the
transformation of the raw value and the single blessed IRI spelling, so an
identifier minted from instance data compares equal, as RDF requires, to the
same identity spelled anywhere else. A value outside the source grammar of
the named scheme is a resolution error. The supported names are:

| Name | Source Grammar | Minted Identity |
|------|----------------|-----------------|
| `mailto` | An [RFC 5321](https://www.rfc-editor.org/rfc/rfc5321) `Mailbox`, such as `gorby%kremvax@example.com` | Its [RFC 6068](https://www.rfc-editor.org/rfc/rfc6068) `mailto` IRI, such as `mailto:gorby%25kremvax@example.com`, with reserved characters percent encoded and the domain name lowercased |
| `acct` | An [RFC 7565](https://www.rfc-editor.org/rfc/rfc7565) `user@host` account, such as `juliet@capulet.example@shoppingsite.example` | Its `acct` IRI, such as `acct:juliet%40capulet.example@shoppingsite.example`, with reserved characters in the user part percent encoded and the host lowercased |

For example, consider the following product catalog schema, which validates
products and maps them to [schema.org](https://schema.org) at the same time:

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "x-jsonld-type": "https://schema.org/Product",
  "properties": {
    "sku": {
      "type": "string",
      "x-jsonld-id": "https://schema.org/sku"
    },
    "name": {
      "type": "string",
      "x-jsonld-id": "https://schema.org/name"
    },
    "releaseDate": {
      "type": "string",
      "format": "date",
      "x-jsonld-id": "https://schema.org/releaseDate",
      "x-jsonld-datatype": "http://www.w3.org/2001/XMLSchema#date"
    },
    "keywords": {
      "type": "array",
      "x-jsonld-id": "https://schema.org/keywords",
      "x-jsonld-container": "@list",
      "items": { "type": "string" }
    },
    "weight": {
      "type": "number",
      "x-jsonld-id": "https://schema.org/weight",
      "x-jsonld-value": "https://schema.org/value",
      "x-jsonld-type": "https://schema.org/QuantitativeValue",
      "x-jsonld-constants": {
        "https://schema.org/unitCode": "KGM"
      }
    },
    "manufacturer": {
      "type": "object",
      "x-jsonld-id": "https://schema.org/manufacturer",
      "x-jsonld-type": "https://schema.org/Organization",
      "properties": {
        "name": {
          "type": "string",
          "x-jsonld-id": "https://schema.org/name"
        },
        "url": {
          "type": "string",
          "x-jsonld-id": "https://schema.org/sameAs",
          "x-jsonld-self": "{+this}"
        }
      }
    }
  }
}
```

Also consider a JSON instance called `instance.json` that looks like this:

```json
{
  "sku": "ABC-123",
  "name": "Vacuum Robot",
  "releaseDate": "2026-01-15",
  "keywords": [ "vacuum", "robot" ],
  "weight": 2.5,
  "manufacturer": {
    "name": "ACME",
    "url": "https://acme.example.com"
  }
}
```

We can promote the instance to Linked Data as follows:

```sh
jsonschema rdf schema.json instance.json
```

The resulting document, which will be printed to standard output in expanded
JSON-LD form, would look like this:

```json
[
  {
    "@type": [ "https://schema.org/Product" ],
    "https://schema.org/keywords": [
      { "@list": [ { "@value": "vacuum" }, { "@value": "robot" } ] }
    ],
    "https://schema.org/manufacturer": [
      {
        "@type": [ "https://schema.org/Organization" ],
        "https://schema.org/name": [ { "@value": "ACME" } ],
        "https://schema.org/sameAs": [ { "@id": "https://acme.example.com" } ]
      }
    ],
    "https://schema.org/name": [ { "@value": "Vacuum Robot" } ],
    "https://schema.org/releaseDate": [
      { "@value": "2026-01-15", "@type": "http://www.w3.org/2001/XMLSchema#date" }
    ],
    "https://schema.org/sku": [ { "@value": "ABC-123" } ],
    "https://schema.org/weight": [
      {
        "@type": [ "https://schema.org/QuantitativeValue" ],
        "https://schema.org/value": [ { "@value": 2.5 } ],
        "https://schema.org/unitCode": [ { "@value": "KGM" } ]
      }
    ]
  }
]
```

Note how the scalar `weight` was promoted to a node of its own:
`x-jsonld-value` carries the number as a literal under the declared
predicate, `x-jsonld-type` types the promoted node, and `x-jsonld-constants`
merges constant properties, such as the measurement unit, that have no
counterpart in the instance.

To instead produce the idiomatic compacted JSON-LD consumed by search engines
and JSON-LD-aware clients, provide a context file such as this `context.json`:

```json
{
  "@context": {
    "@vocab": "https://schema.org/",
    "xsd": "http://www.w3.org/2001/XMLSchema#",
    "keywords": { "@id": "https://schema.org/keywords", "@container": "@list" },
    "releaseDate": { "@id": "https://schema.org/releaseDate", "@type": "xsd:date" },
    "sameAs": { "@id": "https://schema.org/sameAs", "@type": "@id" }
  }
}
```

We can then compact the output against that context as follows:

```sh
jsonschema rdf schema.json instance.json --compact context.json
```

The resulting document would look like this:

```json
{
  "@type": "Product",
  "keywords": [ "vacuum", "robot" ],
  "manufacturer": {
    "@type": "Organization",
    "name": "ACME",
    "sameAs": "https://acme.example.com"
  },
  "name": "Vacuum Robot",
  "releaseDate": "2026-01-15",
  "sku": "ABC-123",
  "weight": {
    "@type": "QuantitativeValue",
    "value": 2.5,
    "unitCode": "KGM"
  },
  "@context": {
    "@vocab": "https://schema.org/",
    "xsd": "http://www.w3.org/2001/XMLSchema#",
    "keywords": { "@id": "https://schema.org/keywords", "@container": "@list" },
    "releaseDate": { "@id": "https://schema.org/releaseDate", "@type": "xsd:date" },
    "sameAs": { "@id": "https://schema.org/sameAs", "@type": "@id" }
  }
}
```

Examples
--------

### Turn a JSON instance into expanded JSON-LD

```sh
jsonschema rdf path/to/schema.json path/to/instance.json
```

### Turn a YAML instance into expanded JSON-LD

```sh
jsonschema rdf path/to/schema.json path/to/instance.yaml
```

### Turn a JSON instance from standard input into expanded JSON-LD

```sh
cat path/to/instance.json | jsonschema rdf path/to/schema.json -
```

### Flatten the JSON-LD output

```sh
jsonschema rdf path/to/schema.json path/to/instance.json --flatten
```

### Compact the JSON-LD output against a context

```sh
jsonschema rdf path/to/schema.json path/to/instance.json --compact path/to/context.json
```

### Flatten and compact the JSON-LD output at once

```sh
jsonschema rdf path/to/schema.json path/to/instance.json --flatten --compact path/to/context.json
```

### Turn a JSON instance into JSON-LD with a schema that imports a local schema

```sh
jsonschema rdf path/to/schema.json path/to/instance.json --resolve path/to/external.json
```

### Turn a JSON instance into JSON-LD with a schema that imports a directory of schemas

```sh
jsonschema rdf path/to/schema.json path/to/instance.json --resolve path/to/schemas/
```

### Turn a JSON instance into JSON-LD while optimising for speed

```sh
jsonschema rdf path/to/schema.json path/to/instance.json --fast
```

### Turn a JSON instance into JSON-LD treating `format` as an assertion

```sh
jsonschema rdf path/to/schema.json path/to/instance.json --format-assertion
```

### Turn a JSON instance into JSON-LD while enabling HTTP resolution

```sh
jsonschema rdf path/to/schema.json path/to/instance.json --http
```

### Turn a JSON instance into JSON-LD with a custom HTTP header

```sh
jsonschema rdf path/to/schema.json path/to/instance.json --http --header "Authorization: Bearer TOKEN"
```
