Canonicalize
============

```sh
jsonschema canonicalize <schema.json|.yaml>
  [--http/-h] [--verbose/-v] [--resolve/-r <schemas-or-directories> ...]
  [--default-dialect/-d <uri>]
```

JSON Schema is an extremely expressive schema language. As such, schema authors
can express the same constraints in a variety of ways, making the process of
statically analyzing schemas complex. This command tackles the problem by
transforming a given JSON Schema into a simpler (but more verbose) normalized
form referred to as *canonical*.

Canonicalization surfaces implicit constraints, simplifies syntax sugar
keywords, and expands schemas into their explicit forms. This is useful for
static analysis tools that need to reason about schema semantics without
handling the full complexity of the JSON Schema specification.

> [!TIP]
> Refer to [Juan Cruz Viotti's dissertation on JSON
> BinPack](https://www.jviotti.com/dissertation.pdf) for how JSON Schema
> canonicalization was originally defined.

Examples
--------

For example, consider the following simple schema:

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "properties": {
    "foo": { "type": "string" }
  }
}
```

The canonicalization process will result in something like this:

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "anyOf": [
    {
      "enum": [ null ]
    },
    {
      "enum": [ false, true ]
    },
    {
      "type": "object",
      "minProperties": 0,
      "properties": {
        "foo": {
          "type": "string",
          "minLength": 0
        }
      }
    },
    {
      "type": "array",
      "minItems": 0
    },
    {
      "type": "string",
      "minLength": 0
    },
    {
      "type": "number"
    },
    {
      "type": "integer",
      "multipleOf": 1
    }
  ]
}
```

### Canonicalize a JSON Schema

```sh
jsonschema canonicalize path/to/my/schema.json
```

### Canonicalize a JSON Schema with a local reference

```sh
jsonschema canonicalize path/to/my/schema.json --resolve path/to/external.json
```

### Canonicalize a JSON Schema while enabling HTTP resolution

```sh
jsonschema canonicalize path/to/my/schema.json --http
```

### Canonicalize a JSON Schema with no `$schema` keyword

If the schema does not declare a `$schema` keyword, you need to specify the
default dialect to use:

```sh
jsonschema canonicalize path/to/my/schema.json \
  --default-dialect "https://json-schema.org/draft/2020-12/schema"
```
