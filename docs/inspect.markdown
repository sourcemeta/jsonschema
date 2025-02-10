Framing
=======

```sh
jsonschema inspect <schema.json|.yaml> [--json/-j] [--verbose/-v]
```

To evaluate a schema, an implementation will first scan it to determine the
dialects and keywords in use, walk over its valid subschemas, and resolve URI
references between them. We refer to this
[reconnaissance](https://en.wikipedia.org/wiki/Reconnaissance) process as
"framing". The JSON Schema CLI offers a `inspect` command so you can "see through
the eyes" of a JSON Schema implementation previous to the evaluation step. This
is often useful for debugging purposes.

Examples
--------

For example, consider the following schema that includes a local reference:

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com",
  "$ref": "#/$defs/string",
  "$defs": { "string": { "type": "string" } }
}
```

The framing process will result in the following entries that capture the
reference:

```
...
(POINTER) URI: https://example.com#/$defs/string/type
    Type             : Static
    Root             : https://example.com
    Pointer          : /$defs/string/type
    Base             : https://example.com
    Relative Pointer : /$defs/string/type
    Dialect          : https://json-schema.org/draft/2020-12/schema
...
(POINTER) URI: https://example.com#/$ref
    Type             : Static
    Root             : https://example.com
    Pointer          : /$ref
    Base             : https://example.com
    Relative Pointer : /$ref
    Dialect          : https://json-schema.org/draft/2020-12/schema
...
(REFERENCE) URI: /$ref
    Type             : Static
    Destination      : https://example.com#/$defs/string
    - (w/o fragment) : https://example.com
    - (fragment)     : /$defs/string
```

### Inspect a JSON Schema

```sh
jsonschema inspect path/to/my/schema.json
```

### Inspect a JSON Schema and output result as a JSON document

```sh
jsonschema inspect path/to/my/schema.json --json
```
