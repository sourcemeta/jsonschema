Codegen
=======

```sh
jsonschema codegen <schema.json|.yaml> --target/-t <target> [--name/-n <name>]
  [--json/-j] [--verbose/-v] [--debug/-g]
  [--resolve/-r <schemas-or-directories>]
  [--default-dialect/-d <uri>]
```

JSON Schema is a declarative language for defining the structure of JSON data.
The JSON Schema CLI offers a `codegen` command that generates type definitions
from a JSON Schema, allowing you to use the schema as a source of truth for
your application's data types.

**This command is experimental. We are actively working on this functionality
and things might break or change without notice. We are actively seeking
feedback to make this better. Please open an issue at
https://github.com/sourcemeta/jsonschema/issues with any problems you find.**

> [!WARNING]
> This command currently only supports JSON Schema 2020-12 and TypeScript as a
> target language. We will extend support to other programming languages and
> dialects once we make the foundations stable.

Keyword Support
---------------

Not every JSON Schema keyword maps directly to type system constructs. This
implementation aims to provide complete structural typing, and you are expected
to use a JSON Schema validator at runtime to enforce remaining constraints.

| Vocabulary | Keyword | TypeScript |
|------------|---------|------------|
| Core (2020-12) | `$schema` | Yes |
| Core (2020-12) | `$id` | Yes |
| Core (2020-12) | `$ref` | Yes |
| Core (2020-12) | `$defs` | Yes |
| Core (2020-12) | `$anchor` | Yes |
| Core (2020-12) | `$dynamicAnchor` | Yes |
| Core (2020-12) | `$dynamicRef` | Yes |
| Core (2020-12) | `$vocabulary` | Ignored |
| Core (2020-12) | `$comment` | Ignored |
| Applicator (2020-12) | `properties` | Yes |
| Applicator (2020-12) | `additionalProperties` | Partial (language limitations) |
| Applicator (2020-12) | `items` | Yes |
| Applicator (2020-12) | `prefixItems` | Yes |
| Applicator (2020-12) | `anyOf` | Yes |
| Applicator (2020-12) | `patternProperties` | Partial (language limitations) |
| Applicator (2020-12) | `propertyNames` | Ignored |
| Applicator (2020-12) | `dependentSchemas` | Pending |
| Applicator (2020-12) | `contains` | Ignored |
| Applicator (2020-12) | `allOf` | Yes |
| Applicator (2020-12) | `oneOf` | Partial (language limitations) |
| Applicator (2020-12) | `not` | Cannot support |
| Applicator (2020-12) | `if` | Partial (language limitations) |
| Applicator (2020-12) | `then` | Partial (language limitations) |
| Applicator (2020-12) | `else` | Partial (language limitations) |
| Validation (2020-12) | `type` | Yes |
| Validation (2020-12) | `enum` | Yes |
| Validation (2020-12) | `required` | Yes |
| Validation (2020-12) | `const` | Yes |
| Validation (2020-12) | `minLength` | Ignored |
| Validation (2020-12) | `maxLength` | Ignored |
| Validation (2020-12) | `pattern` | Ignored |
| Validation (2020-12) | `minimum` | Ignored |
| Validation (2020-12) | `maximum` | Ignored |
| Validation (2020-12) | `exclusiveMinimum` | Ignored |
| Validation (2020-12) | `exclusiveMaximum` | Ignored |
| Validation (2020-12) | `multipleOf` | Ignored |
| Validation (2020-12) | `minProperties` | Ignored |
| Validation (2020-12) | `maxProperties` | Ignored |
| Validation (2020-12) | `dependentRequired` | Pending |
| Validation (2020-12) | `minItems` | Ignored |
| Validation (2020-12) | `maxItems` | Ignored |
| Validation (2020-12) | `minContains` | Ignored |
| Validation (2020-12) | `maxContains` | Ignored |
| Validation (2020-12) | `uniqueItems` | Ignored |
| Unevaluated (2020-12) | `unevaluatedItems` | Pending |
| Unevaluated (2020-12) | `unevaluatedProperties` | Pending |
| Meta-Data (2020-12) | `title` | Ignored |
| Meta-Data (2020-12) | `description` | Ignored |
| Meta-Data (2020-12) | `default` | Ignored |
| Meta-Data (2020-12) | `deprecated` | Ignored |
| Meta-Data (2020-12) | `examples` | Ignored |
| Meta-Data (2020-12) | `readOnly` | Ignored |
| Meta-Data (2020-12) | `writeOnly` | Ignored |
| Format Annotation (2020-12) | `format` | Ignored |
| Format Assertion (2020-12) | `format` | Ignored |
| Content (2020-12) | `contentEncoding` | Ignored |
| Content (2020-12) | `contentMediaType` | Ignored |
| Content (2020-12) | `contentSchema` | Ignored |

Examples
--------

For example, consider the following schema that describes a person:

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "properties": {
    "name": { "type": "string" },
    "age": { "type": "integer" }
  },
  "required": [ "name" ]
}
```

The codegen command will generate the following TypeScript types:

```typescript
export type PersonName = string;

export type PersonAge = number;

export interface Person {
  "name": PersonName;
  "age"?: PersonAge;
  [key: string]: unknown | undefined;
}
```

### Generate TypeScript types from a JSON Schema

```sh
jsonschema codegen path/to/my/schema.json --target typescript
```

### Generate TypeScript types with a custom type name prefix

By default, generated types use `Schema` as the prefix. You can specify a
custom prefix using the `--name/-n` option:

```sh
jsonschema codegen path/to/my/schema.json --target typescript --name Person
```

### Output the generated code as JSON

```sh
jsonschema codegen path/to/my/schema.json --target typescript --json
```
