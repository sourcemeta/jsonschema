Formatting
==========

```sh
jsonschema fmt [schemas-or-directories...]
  [--check/-c] [--verbose/-v] [--extension/-e <extension>]
  [--ignore/-i <schemas-or-directories>] [--keep-ordering/-k]
  [--indentation/-n <spaces>]
```

Schemas are code. As such, they are expected follow consistent stylistic
conventions.  Just as code-formatters like
[clang-format](https://clang.llvm.org/docs/ClangFormat.html), JavaScript's
[prettier](https://prettier.io/), and
[rustfmt](https://github.com/rust-lang/rustfmt), the JSON Schema CLI offers a
`fmt` command to format schemas based on industry-standard conventions and to
check their adherence on a continuous integration environment.

**This command does not support YAML schemas yet.**

Examples
--------

For example, consider this fictitious JSON Schema with inconsistent
indentation, spacing, keyword ordering, and more:

```json
{ "$schema":"https://json-schema.org/draft/2020-12/schema",
      "type": "string","pattern": "^(?!0000)\\d{4}$",
  "$id": "https://example.com/iso8601/v1.json",
      "title":    "ISO 8601 four-digit year (YYYY)" }
```

After formatting it, the JSON Schema looks like this:

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/iso8601/v1.json",
  "title": "ISO 8601 four-digit year (YYYY)",
  "type": "string",
  "pattern": "^(?!0000)\\d{4}$"
}
```

### Format JSON Schemas in-place

```sh
jsonschema fmt path/to/my/schema_1.json path/to/my/schema_2.json
```

### Format JSON Schemas in-place while preserving keyword ordering

```sh
jsonschema fmt path/to/my/schema_1.json path/to/my/schema_2.json --keep-ordering
```

### Format JSON Schemas in-place while indenting on 4 spaces

```sh
jsonschema fmt path/to/my/schema_1.json path/to/my/schema_2.json --indentation 4
```

### Format every `.json` file in a given directory (recursively)

```sh
jsonschema fmt path/to/schemas/
```

### Format every `.json` file in the current directory (recursively)

```sh
jsonschema fmt
```

### Format every `.json` file in a given directory while ignoring another

```sh
jsonschema fmt path/to/schemas/ --ignore path/to/schemas/nested
```

### Format every `.schema.json` file in the current directory (recursively)

```sh
jsonschema fmt --extension .schema.json
```

### Check that a single JSON Schema is properly formatted

```sh
jsonschema fmt path/to/my/schema.json --check
```
