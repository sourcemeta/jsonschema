Testing
=======

> [!WARNING]
> Only JSON Schema Draft 4, Draft 6, and Draft 7 are supported at this point in
> time. We have plans to support *every* dialect of JSON Schema from Draft 0 to
> Draft 2020-12 soon.

```sh
jsonschema test [schemas-or-directories...]
  [--http/-h] [--verbose/-v] [--resolve/-r <schemas-or-directories> ...]
  [--extension/-e <extension>] [--ignore/-i <schemas-or-directories>]
```

Schemas are code. As such, you should run an automated unit testing suite
against them. Just like popular test frameworks like [Jest](https://jestjs.io),
[GoogleTest](https://google.github.io/googletest/), and
[PyTest](https://docs.pytest.org), the JSON Schema CLI provides a
schema-oriented test runner inspired by the [official JSON Schema test
suite](https://github.com/json-schema-org/JSON-Schema-Test-Suite).

**If you want to validate that a schema adheres to its metaschema, use the
[`metaschema`](./metaschema.markdown) command instead.**

Examples
--------

To create a test definition, you must write JSON documents that look like this:

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
    }
  ]
}
```

> [!TIP]
> You can test different portions of a large schema by passing a schema URI
> that contains a JSON Pointer in the `target` property. For example:
> `https://example.com/my-big-schema#/definitions/foo`.

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
