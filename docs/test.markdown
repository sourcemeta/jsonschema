Testing
=======

> [!WARNING]
> JSON Schema Draft 2 and older are not supported at this point in time.

```sh
jsonschema test [schemas-or-directories...]
  [--http/-h] [--verbose/-v] [--debug/-g]
  [--header/-H "<name>: <value>"]
  [--resolve/-r <schemas-or-directories> ...]
  [--extension/-e <extension>] [--ignore/-i <schemas-or-directories>]
  [--default-dialect/-d <uri>] [--json/-j] [--format-assertion/-F]
  [--jobs/-J <count>] [--configuration/-C <path>] [--color auto|always|never]
```

> [!NOTE]
> See [Resolving External References](./guides/resolution.markdown) for every way of
> making referenced schemas available, including how to handle a reference whose
> URI differs from the identifier the target schema declares.

Schemas are code. As such, you should run an automated unit testing suite
against them. Just like popular test frameworks like [Jest](https://jestjs.io),
[GoogleTest](https://google.github.io/googletest/), and
[PyTest](https://docs.pytest.org), the JSON Schema CLI provides a
schema-oriented test runner inspired by the [official JSON Schema test
suite](https://github.com/json-schema-org/JSON-Schema-Test-Suite).

**If you want to validate that a schema adheres to its metaschema, use the
[`metaschema`](./metaschema.markdown) command instead.**

Pass `--json` to output results in [CTRF (Common Test Report
Format)](https://ctrf.io), a standardized JSON format for test results that
integrates with CI/CD tools and test result dashboards.

Test suites run in parallel, defaulting to as many jobs as there are CPU
cores. Use `--jobs/-J` to control the level of parallelism. Note that when
running more than one job, suites report in completion order rather than in
input order, unless you pass `--json`, which always reports in input order.
Pass `--jobs 1` to run suites serially in input order. Peak
memory usage grows with the number of jobs, as each job holds a compiled
schema in memory. Keep in mind that when combined with `--verbose` or
`--debug`, log lines from concurrent jobs may interleave on standard error.

Writing tests
-------------

To test a schema, you define one or more test suite (i.e. collections of tests)
as JSON files that follow a specific format:

- `target`: The URI of the schema you want to test, _which must be imported
  into the resolution context using the `--resolve` or `--http` options_.

  If the `target` is relative, it will be interpreted as a file path relative
  to the test file location. A test document read from standard input has no
  location of its own, so its relative paths are interpreted against the
  current working directory instead.

  This property may also be set to a non-empty array of URIs, in which case
  every test case in `tests` will be run against each of the listed schemas
  in turn. This is convenient for asserting that a single set of instances
  is accepted (or rejected) the same way by multiple related schemas.

- `tests`: An array of tests you want to run.

> [!TIP]
> You can test different portions of a large schema by passing a schema URI
> that contains a JSON Pointer in the `target` property. For example:
> `https://example.com/my-big-schema#/definitions/foo`.

> [!IMPORTANT]
> A `target` is a URI, fragment included, so a character that a URI fragment
> cannot carry is written percent-encoded. This comes up for a Schema Object
> that sits under a templated path, whose braces become `%7B` and `%7D`:
> `./openapi.json#/paths/~1pets~1%7BpetId%7D/get/requestBody/content/application~1json/schema`.

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

Additionally, if the schema under test is annotated with the `x-jsonld-*`
keywords of the [`rdf`](./rdf.markdown) command, a test that sets `valid` to
`true` may declare either of the following properties, but not both, to also
assert on the promotion of the instance to JSON-LD:

- `rdf`: The expected promotion of the instance to JSON-LD in [expanded
  form](https://www.w3.org/TR/json-ld11/#expanded-document-form), which is
  always an array
- `rdfPath`: The expected promotion of the instance to JSON-LD in expanded
  form, loaded from an external file instead

The comparison takes place against the canonical expanded form, so a
convenient way to author the expectation is to run the
[`rdf`](./rdf.markdown) command and copy its output. Note that unlike that
command, the test runner does not reject schemas whose base dialect predates
JSON Schema 2019-09. The `x-jsonld-*` keywords of older schemas do not emit
annotations, so their instances always promote to the empty array `[]`.

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

OpenAPI Descriptions
--------------------

A `target` names the schema under test wherever it lives, so the document at
its base does not have to be a schema itself. If it is an
[OpenAPI](https://spec.openapis.org/oas/latest.html) spec v3.1 or v3.2
description, the `test` command reads it as one and runs the test cases against
the Schema Object that the fragment names:

```json
{
  "target": "./openapi.json#/components/schemas/Person",
  "tests": [
    {
      "description": "I expect to pass",
      "valid": true,
      "data": { "name": "Juan" }
    }
  ]
}
```

A description that names itself with `$self` answers to that name rather than
to where it was read from, so a `target` may spell either. Any other revision,
such as v3.0, is rejected rather than read as a schema.

> [!IMPORTANT]
> A Schema Object is named by a pointer into the description that holds it, even
> when it declares an `$id` of its own. What answers to a URI is a document,
> not a schema sitting inside one, so `"target": "https://example.com/person"`
> finds nothing unless a schema of that identifier was imported in its own
> right.

Use `--resolve/-r` for the other documents a description spans, and for the
description itself when a `target` names it by URI, exactly as you would for
schemas. One suite may mix the two kinds of target freely, as each `target` is
resolved on its own.

A reference written as a relative file path needs no importing at all, as it is
resolved against the description that spells it and read from disk. That holds
whether a Schema Object reaches another schema or the description reaches
another description. A schema that arrives through a reference is named where it
lands rather than where it was written, since a `target` addresses the
description once everything it spans has been embedded into it. So a Path Item
brought in from another document is reached under the Components Object:

```json
{
  "target": "./openapi.json#/components/pathItems/people/get/requestBody/content/application~1json/schema"
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

### Run a single test definition with a custom HTTP header

```sh
jsonschema test path/to/test.json \
  --http --header "Authorization: Bearer $REGISTRY_TOKEN"
```

### Run a single test definition importing a single local schema

```sh
jsonschema test path/to/test.json --resolve path/to/external.json
```

### Run a single test definition importing a directory of `.schema.json` schemas

```sh
jsonschema test path/to/test.json --resolve path/to/schemas --extension schema.json
```

### Run a single test definition targeting a schema of an OpenAPI description

```sh
jsonschema test path/to/my/test.json
```

### Run a single test definition targeting an OpenAPI description by URI

```sh
jsonschema test path/to/my/test.json --resolve path/to/my/openapi.json
```

### Run a single test definition forcing every `format` to assert

The `--format-assertion`/`-F` option forces every `format` keyword in the
schema under test to behave as an assertion regardless of dialect or
vocabulary, which lets tests cover invalid-format cases without changing the
schema.

```sh
jsonschema test path/to/test.json --resolve path/to/schema.json --format-assertion
```
