Canonicalize
============

```sh
jsonschema canonicalize <schema.json>
```

JSON Schema is an extremely expressive schema language. As such, schema authors
can express the same constraints in a variety of ways, making the process of
statically analyzing schemas complex. This command attempts to tackle the
problem by transforming a given JSON Schema into a simpler (but more verbose)
normalized form referred to as _canonical_.

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

```
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "anyOf": [
    {
      "enum": [
        null
      ]
    },
    {
      "enum": [
        false,
        true
      ]
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
      "type": "number",
      "multipleOf": 1
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
