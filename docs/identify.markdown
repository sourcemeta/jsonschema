Identify
========

```sh
jsonschema identify <schema.json> [--relative-from/-f <uri>] [--verbose/-v]
```

A schema may be associated with a URI through the use of keywords like `$id` or
its predecessor `id`. This command takes a JSON Schema as input and prints to
standard output the identifying URI of the schema. This command heuristically
handles the case where you don't know the base dialect of the schema. For
convenience, you may provide a base URI to resolve the schema identity URI
against.

Examples
--------

For example, consider the following schema identified as
`https://example.com/foo/bar`.

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/foo/bar"
}
```

We can interact with the identifier URI of this schema as follows:

```sh
$ jsonschema identify schema.json
https://example.com/foo/bar

$ jsonschema identify schema.json --relative-from "https://example.com"
/foo/bar
```

### Identify a JSON Schema

```sh
jsonschema identify path/to/my/schema.json
```

### Identify a JSON Schema resolving the URI relative to a given base

```sh
jsonschema identify path/to/my/schema.json --relative-from "https://example.com"
```
