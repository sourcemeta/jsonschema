Testing
=======

> [!WARNING]
> JSON Schema Draft 3 and older are not supported at this point in time.

```sh
jsonschema test [schemas-or-directories...]
  [--http/-h] [--verbose/-v] [--resolve/-r <schemas-or-directories> ...]
  [--extension/-e <extension>] [--ignore/-i <schemas-or-directories>]
  [--default-dialect/-d <uri>] [--json/-j]
```

Schemas are code. As such, you should run an automated unit testing suite
against them. Just like popular test frameworks like [Jest](https://jestjs.io),
[GoogleTest](https://google.github.io/googletest/), and
[PyTest](https://docs.pytest.org), the JSON Schema CLI provides a
schema-oriented test runner inspired by the [official JSON Schema test
suite](https://github.com/json-schema-org/JSON-Schema-Test-Suite).

**If you want to validate that a schema adheres to its metaschema, use the
[`metaschema`](./metaschema.markdown) command instead.**

Writing tests
-------------

To test a schema, you define one or more test suite (i.e. collections of tests)
as JSON files that follow a specific format:

- `target`: The URI of the schema you want to test, _which must be imported
  into the resolution context using the `--resolve` or `--http` options_.

  If the `target` is relative, it will be interpreted as a file path relative
  to the test file location. For security reasons, the file path still needs be
  imported into the resolution context using `--resolve`.

- `tests`: An array of tests you want to run.

> [!TIP]
> You can test different portions of a large schema by passing a schema URI
> that contains a JSON Pointer in the `target` property. For example:
> `https://example.com/my-big-schema#/definitions/foo`.

Every item in the `tests` array must be an object with the following
properties:

- `valid`: A boolean that determines whether the given instance is expected to
  be valid or not against the target schema. In other words, if this property
  is `true`, the test passes when the instance successfully validates against
  the schema.  Conversely, if this property is `false`, the test passes when
  the instance does NOT validate against the schema
- `description`: An optional string property to make test output more readable

And either of the following properties, but not both:

- `data`: The instance that you want to test against the target schema.
- `dataPath`: The instance that you want to test against the target schema,
  loaded from an external file instead

For example, here is a minimal test suite that expects an object with two
properties (`foo` and `bar`) to successfully validate against the target schema
`https://example.com/my-schema-id`:

```json
{
  "target": "https://example.com/my-schema-id",
  "tests": [
    {
      "description": "I expect to pass",
      "valid": true,
      "data": {
        "foo": 1,
        "bar": 1
      }
    }
  ]
}
```

Examples
--------

This is a test suite definition that runs a few test cases against the official
JSON Schema Draft 4 meta-schema. The first test asserts that the instance `{}`
is valid. The second test asserts that a schema where the `type` keyword is set
to an integer is invalid. The third test asserts that an instance loaded from a
relative path is valid against the schema:

```json
{
  "target": "http://json-schema.org/draft-04/schema#",
  "$comment": "An arbitrary comment! Put whatever you want here",
  "tests": [
    {
      "description": "The empty object is valid",
      "valid": true,
      "data": {}
    },
    {
      "description": "The `type` keyword must be a string",
      "valid": false,
      "data": {
        "type": 1
      }
    },
    {
      "description": "Load from an external file, relative to the test",
      "valid": true,
      "dataPath": "../my-data.json"
    }
  ]
}
```

Assuming this file is saved as `test/draft4.json`, you can run it as follows:

```sh
jsonschema test test/draft4.json
```

### Run a single test definition

```sh
jsonschema test path/to/test.json
```

### Run every `.json` test definition in a given directory (recursively)

```sh
jsonschema test path/to/tests/
```

### Run every `.json` test definition in the current directory (recursively)

```sh
jsonschema test
```

### Run every `.json` test definition in the current directory while ignoring another

```sh
jsonschema test --ignore dist
```

### Run every `.test.json` test definition in the current directory (recursively)

```sh
jsonschema test --extension .test.json
```

### Run a single test definition enabling HTTP resolution

```sh
jsonschema test path/to/test.json --http
```

### Run a single test definition importing a single local schema

```sh
jsonschema test path/to/test.json --resolve path/to/external.json
```

### Run a single test definition importing a directory of `.schema.json` schemas

```sh
jsonschema test path/to/test.json --resolve path/to/schemas --extension schema.json
```
