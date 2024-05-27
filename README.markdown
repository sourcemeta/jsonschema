![JSON Schema](./assets/banner.png)

The command-line tool for working with [JSON Schema](https://json-schema.org),
the world most popular schema language. It is designed to solve the most common
pain points JSON Schema developers face when maintaining **repositories of
schemas**, both during local development and when integrating with CI/CD
pipelines.

> Do you want to level up your JSON Schema skills? Check out
> [learnjsonschema.com](https://www.learnjsonschema.com), our growing JSON
> Schema documentation website, and our O'Reilly book [Unifying Business, Data,
> and Code: Designing Data Products with JSON
> Schema](https://www.oreilly.com/library/view/unifying-business-data/9781098144999/).

***

**This project is under heavy development. Some features are partly available
and may contain bugs. Please [share
feedback](https://github.com/Intelligence-AI/jsonschema/issues/new) and give us
a star show your support!**

***

Installation
------------

The JSON Schema CLI is written using C++ and CMake, and supports macOS, Windows,
and GNU/Linux. Under the hood, it relies on the powerful [JSON
Toolkit](https://github.com/sourcemeta/jsontoolkit) library.

From source (with [CMake](https://cmake.org/)):

```sh
git clone https://github.com/intelligence-ai/jsonschema
cmake -S . -B ./build -DCMAKE_BUILD_TYPE:STRING=Release
cmake --build ./build --config Release --parallel 4
cmake --install ./build --prefix <prefix> \
  --config Release --verbose --component intelligence_jsonschema
```

Where `<prefix>` can be any destination prefix of your choosing, such as `/opt`
or `/usr/local`.

Validating
----------

```sh
jsonschema validate <path/to/schema.json> <path/to/instance.json>
```

The most popular use case of JSON Schema is to validate JSON documents. The
JSON Schema CLI offers a `validate` command to do exactly that.

### Examples

#### Validate a JSON instance

```sh
jsonschema validate path/to/my/schema.json path/to/my/instance.json
```

Formatting
----------

```sh
jsonschema fmt <schemas-or-directories...> [--check|-c]
```

Schemas are code. As such, they are expected follow consistent stylistic
conventions.  Just as code-formatters like
[clang-format](https://clang.llvm.org/docs/ClangFormat.html), JavaScript's
[prettier](https://prettier.io/), and
[rustfmt](https://github.com/rust-lang/rustfmt), the JSON Schema CLI offers a
`fmt` command to format schemas based on industry-standard conventions and to
check their adherence on a continuous integration environment.

### Examples

For example, consider this fictitious JSON Schema with inconsistent identation,
spacing, keyword ordering, and more:

```json
{ "$schema":"https://json-schema.org/draft/2020-12/schema",
      "type": "string","pattern": "^(?!0000)\\d{4}$",
  "$id": "https://schemas.intelligence.ai/std/iso8601-year/v1.json",
      "title":    "ISO 8601 four-digit year (YYYY)" }
```

After formatting it, the JSON Schema looks like this:

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://schemas.intelligence.ai/std/iso8601-year/v1.json",
  "title": "ISO 8601 four-digit year (YYYY)",
  "type": "string",
  "pattern": "^(?!0000)\\d{4}$"
}
```

#### Format JSON Schemas in-place

```sh
jsonschema fmt path/to/my/schema_1.json path/to/my/schema_2.json
```

#### Format every `.json` file in a given directory (recursively)

```sh
jsonschema fmt path/to/schemas/
```

#### Format every `.json` file in the current directory (recursively)

```sh
jsonschema fmt
```

#### Check that a single JSON Schema is properly formatted

```sh
jsonschema fmt path/to/my/schema.json --check
```

Linting
-------

```sh
jsonschema lint <schemas-or-directories...> [--fix|-f]
```

JSON Schema is a surprisingly expressive schema language. Like with traditional
programming languages, writing efficient and maintainable schemas take
experience, and there are lots of ways of doing it wrong. To help with this,
the JSON Schema CLI provides a `lint` command that can check your schemas
against various common anti-patterns and automatically fix many of them.

### Examples

#### Lint JSON Schemas in-place

```sh
jsonschema lint path/to/my/schema_1.json path/to/my/schema_2.json
```

#### Lint every `.json` file in a given directory (recursively)

```sh
jsonschema lint path/to/schemas/
```

#### Lint every `.json` file in the current directory (recursively)

```sh
jsonschema lint
```

#### Fix lint warnings on a single schema

```sh
jsonschema lint path/to/my/schema.json --fix
```

Framing
-------

```sh
jsonschema frame <path/to/schema.json>
```

To evaluate a schema, an implementation will first scan it to determine the
dialects and keywords in use, walk over its valid subschemas, and resolve URI
references between them. We refer to this
[reconnaissance](https://en.wikipedia.org/wiki/Reconnaissance) process as
"framing". The JSON Schema CLI offers a `frame` command so you can "see through
the eyes" of a JSON Schema implementation previous to the evaluation step. This
is often useful for debugging purposes.

### Examples

For example, consider the following schema with a local reference:

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
(LOCATION) URI: https://example.com#/$defs/string/type
    Schema           : https://example.com
    Pointer          : /$defs/string/type
    Base URI         : https://example.com
    Relative Pointer : /$defs/string/type
    Dialect          : https://json-schema.org/draft/2020-12/schema
...
(LOCATION) URI: https://example.com#/$ref
    Schema           : https://example.com
    Pointer          : /$ref
    Base URI         : https://example.com
    Relative Pointer : /$ref
    Dialect          : https://json-schema.org/draft/2020-12/schema
...
(REFERENCE) URI: /$ref
    Type             : Static
    Destination      : https://example.com#/$defs/string
    - (w/o fragment) : https://example.com
    - (fragment)     : /$defs/string
```

#### Frame a JSON Schema

```sh
jsonschema frame path/to/my/schema.json
```

Coming Soon
-----------

This project is under heavy development, and we have a lot of cool things in
the oven. In the mean-time, star the project to show your support!

| Feature               | Description                                                                        | Status      |
|-----------------------|------------------------------------------------------------------------------------|-------------|
| Bundling              | Inline remote references to other schemas using JSON Schema Bundling               | Not started |
| Testing               | A test runner for JSON Schema                                                      | Not started |
| Annotating            | Validate a JSON instance against a JSON Schema with annotation extraction support  | Not started |
| Debugging             | Validate a JSON instance against a JSON Schema step by step, like LLDB and GDB     | Not started |
| Upgrading/Downgrading | Convert a JSON Schema into a later or older dialect                                | Not started |

License
-------

This project is governed by the [AGPL-3.0](./LICENSE) copyleft license.
