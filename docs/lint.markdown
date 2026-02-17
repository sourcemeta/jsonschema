Linting
=======

```sh
jsonschema lint [schemas-or-directories...] [--http/-h] [--fix/-f]
  [--format/-m] [--keep-ordering/-k] [--json/-j] [--verbose/-v] [--debug/-g]
  [--resolve/-r <schemas-or-directories> ...]
  [--extension/-e <extension>] [--ignore/-i <schemas-or-directories>]
  [--exclude/-x <rule-name>] [--only/-o <rule-name>] [--list/-l]
  [--rule/-a <rule-schema>]
  [--default-dialect/-d <uri>] [--indentation/-n <spaces>]
```

JSON Schema is a surprisingly expressive schema language. Like with traditional
programming languages, writing efficient and maintainable schemas takes
experience, and there are lots of common pitfalls. Just like popular linters
like [ESLint](https://eslint.org),
[ClangTidy](https://clang.llvm.org/extra/clang-tidy/), and
[PyLint](https://www.pylint.org), the JSON Schema CLI provides a `lint` command
that can check your schemas against various common anti-patterns and
automatically fix many of them.

> [!TIP]
> In comparison to [Spectral](https://github.com/stoplightio/spectral), this
> CLI is exclusively focused on linting JSON Schema whereas Spectral focuses on
> linting API specifications (OpenAPI, AsyncAPI, Arazzo, etc) touching on JSON
> Schema as a byproduct. Therefore, this CLI is expected to have deeper
> coverage of JSON Schema and be also usable in JSON Schema use cases that are
> not related to APIs. If you are working with JSON Schema for API
> specifications, you should make use of both linters together!

**The `--fix/-f` option is not supported when passing YAML schemas.**

**The `--format/-m` option requires `--fix/-f` to be set and is not supported
for YAML schemas.** When `--format/-m` is set, the output file is always
written with proper formatting (equivalent to running `fmt`), even if there
are no lint issues to fix. Use `--keep-ordering/-k` with `--format/-m` to
preserve key ordering during formatting.

> [!NOTE]
> There are linting rules that require compiling and validating instance
> against the given schema. For example, there is a rule to check that the
> instances set in the `examples` and `default` keywords successfully validate
> against the corresponding subschema. In those cases, if your schema has
> external references, you will have to import them using the `--resolve`/`-r`
> options as you would normally do when making use of other commands like
> `validate` and `test`.

Use `--list/-l` to print all the available rules and brief descriptions about
them.

Disabling Rules
---------------

While you can disable rules globally using the `--exclude/-x` option, you may
want to disable specific rules for individual subschemas. To do this, add the
`x-lint-exclude` keyword to the subschema, set to either a rule name or an
array of rule names to exclude.

For example, if you intentionally want to use `type` alongside `enum` in a
specific property, you can disable the `enum_with_type` rule for just that
subschema:

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "properties": {
    "status": {
      "type": "string",
      "enum": [ "active", "inactive" ],
      "x-lint-exclude": "enum_with_type"
    }
  }
}
```

To disable multiple rules, use an array:

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "properties": {
    "status": {
      "type": "string",
      "enum": [ "active" ],
      "x-lint-exclude": [ "enum_with_type", "enum_to_const" ]
    }
  }
}
```

Custom Rules
------------

You can define custom lint rules as JSON Schemas using the `--rule/-a` option.
Each rule schema must have a `title` keyword (used as the rule name) and
optionally a `description` keyword (used as the rule message). The title must
consist only of lowercase ASCII letters, digits, underscores, or slashes.

When linting, _every subschema in the target schema_ is validated as a JSON
instance against each custom rule schema (not only the top one). If any
subschema does not conform, the rule fires and reports the validation errors.

For example, create a rule that requires every subschema to declare a `type`:

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "require_type",
  "description": "Every subschema must declare the type keyword",
  "required": [ "type" ]
}
```

Then run:

```sh
jsonschema lint --rule require_type.json path/to/my/schema.json
```

You can pass multiple custom rules:

```sh
jsonschema lint --rule rule1.json --rule rule2.json path/to/my/schema.json
```

Custom rules can also be declared in the
[`jsonschema.json`](./configuration.markdown) configuration file using the
`lint.rules` property.

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

### Lint while disabling certain rules

```sh
jsonschema lint path/to/my/schema.json --exclude enum_with_type --exclude const_with_type
```

### Lint with only a set of preselected rules

```sh
jsonschema lint path/to/my/schema.json --only enum_with_type --only const_with_type
```

### Lint with JSON output

```sh
jsonschema lint path/to/my/schema.json --json
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

### Fix lint warnings on a single schema while indenting on 4 spaces

```sh
jsonschema lint path/to/my/schema.json --fix --indentation 4
```

### Fix lint warnings and format the schema

```sh
jsonschema lint path/to/my/schema.json --fix --format
```

### Fix lint warnings, format, but preserve keyword ordering

```sh
jsonschema lint path/to/my/schema.json --fix --format --keep-ordering
```

### Print a summary of all enabled rules

```sh
jsonschema lint --list
```

### Lint with a custom rule

```sh
jsonschema lint --rule path/to/my/rule.json path/to/my/schema.json
```

### Lint with multiple custom rules

```sh
jsonschema lint --rule rule1.json --rule rule2.json path/to/my/schema.json
```
