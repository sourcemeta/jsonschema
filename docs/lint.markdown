Linting
=======

```sh
jsonschema lint [schemas-or-directories...]
  [--fix/-f] [--verbose/-v] [--extension/-e <extension>]
  [--ignore/-i <schemas-or-directories>]
```

JSON Schema is a surprisingly expressive schema language. Like with traditional
programming languages, writing efficient and maintainable schemas takes
experience, and there are lots of common pitfalls. Just like popular linters
like [ESLint](https://eslint.org),
[ClangTidy](https://clang.llvm.org/extra/clang-tidy/), and
[PyLint](https://www.pylint.org), the JSON Schema CLI provides a `lint` command
that can check your schemas against various common anti-patterns and
automatically fix many of them.

Examples
--------

For example, consider the following schema that asserts that the JSON instance
is the string `foo`:

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "enum": [ "foo" ]
}
```

While this schema is technically correct, the JSON Schema 2020-12 dialect has a
[`const`](https://www.learnjsonschema.com/2020-12/validation/const/) keyword
that better expresses the intention of matching a single possible value.

Running the JSON Schema CLI linter against the previous schema will produce the
following output:

```sh
$ jsonschema lint schema.json
schema.json
     An `enum` of a single value can be expressed as `const` (enum_to_const)
```

Furthermore, running the `lint` command with the `--fix / -f` option will
result in the JSON Schema CLI *automatically* fixing the warning for you.

### Lint multiple schemas in-place

```sh
jsonschema lint path/to/my/schema_1.json path/to/my/schema_2.json
```

### Lint every `.json` file in a given directory (recursively)

```sh
jsonschema lint path/to/schemas/
```

### Lint every `.json` file in a given directory while ignoring another

```sh
jsonschema lint path/to/schemas/ --ignore path/to/schemas/nested
```

### Lint every `.json` file in the current directory (recursively)

```sh
jsonschema lint
```

### Lint every `.schema.json` file in the current directory (recursively)

```sh
jsonschema lint --extension .schema.json
```

### Fix lint warnings on a single schema

```sh
jsonschema lint path/to/my/schema.json --fix
```
