Inspect
=======

```sh
jsonschema inspect <schema.json|.yaml|openapi.json|.yaml> [--json/-j]
  [--verbose/-v] [--debug/-g]
  [--default-dialect/-d <uri>] [--configuration/-C <path>]
  [--color auto|always|never]
```

To evaluate a schema, an implementation will first scan it to determine the
dialects and keywords in use, walk over its valid subschemas, and resolve URI
references between them. The JSON Schema CLI offers a `inspect` command so you
can "see through the eyes" of a JSON Schema implementation previous to the
evaluation step. This is often useful for debugging purposes.

OpenAPI Descriptions
--------------------

If the input is an OpenAPI spec v3.1 or v3.2, the `inspect` command treats it as
an [OpenAPI](https://spec.openapis.org/oas/latest.html) description rather than
as a schema, and inspects it in a similar fashion, reporting on the Objects it
holds, the operations it exposes, and the Schema Objects within it. Any other
revision, such as v3.0, is rejected rather than read as a schema.

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

The inspect process will result in the following entries that capture the
reference:

```
...
(POINTER) URI: https://example.com#/$defs/string/type
    Type              : Static
    Root              : https://example.com
    Pointer           : /$defs/string/type
    Base              : https://example.com
    Relative Pointer  : /$defs/string/type
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            : /$defs/string
...
(POINTER) URI: https://example.com#/$ref
    Type              : Static
    Root              : https://example.com
    Pointer           : /$ref
    Base              : https://example.com
    Relative Pointer  : /$ref
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
...
(REFERENCE) ORIGIN: /$ref
    Type              : Static
    Destination       : https://example.com#/$defs/string
    - (w/o fragment)  : https://example.com
    - (fragment)      : /$defs/string
```

### Inspect a JSON Schema

```sh
jsonschema inspect path/to/my/schema.json
```

### Inspect an OpenAPI description

```sh
jsonschema inspect path/to/my/openapi.json
```

### Inspect a JSON Schema and output result as a JSON document

```sh
jsonschema inspect path/to/my/schema.json --json
```
