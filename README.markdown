![JSON Schema](./assets/banner.png)

The command-line tool for working with [JSON Schema](https://json-schema.org),
the world most popular schema language. It is a comprehensive solution for
maintaining **repositories of schemas** and ensuring their quality, both during
local development and when running on CI/CD pipelines. For example:

- Ensure your schemas are valid
- Unit test your schemas against valid and invalid instances
- Enforce consistent indentation and keyword ordering in your schema files
- Detect and fix common JSON Schema anti-patterns
- Inline external references for conveniently distributing your schemas

[**Check out the documentation to learn more**](#usage)

***

> [!TIP]
> Do you want to level up your JSON Schema skills? Check out
> [learnjsonschema.com](https://www.learnjsonschema.com), our growing JSON
> Schema documentation website, and our O'Reilly book [Unifying Business, Data,
> and Code: Designing Data Products with JSON
> Schema](https://www.oreilly.com/library/view/unifying-business-data/9781098144999/).

***

> [!WARNING]
> This project is under heavy development. Some features are partly available
> and may contain bugs. Please [share
> feedback](https://github.com/Intelligence-AI/jsonschema/issues/new) and give
> us a star show your support!
>
> **Current Limitations:**
>
> - The `validate` and `test` commands only support JSON Schema Draft 4, Draft 6, and Draft 7
> - It is not possible to collect annotations with the `validate` command

What our users are saying
-------------------------

> Amazing product. Very useful for formatting and bundling my schemas, plus it
> surfaced various referencing issues. 10 out of 10!

[@alombarte](https://github.com/alombarte), co-founder of the
[KrakenD](https://www.krakend.io) API Gateway.

Installation
------------

The JSON Schema CLI is written using C++ and [CMake](https://cmake.org/), and
supports macOS, Windows, and GNU/Linux. Under the hood, it relies on the
powerful [JSON Toolkit](https://github.com/sourcemeta/jsontoolkit) library.

### From Homebrew

```sh
brew install intelligence-ai/apps/jsonschema
```

### From GitHub Actions

```yaml
- uses: intelligence-ai/jsonschema@vX.Y.Z
```

Where `X.Y.Z` is replaced with the desired version. For example:

```yaml
- uses: intelligence-ai/jsonschema@v1.1.2
# Then use as usual
- run: jsonschema fmt path/to/schemas --check
```

### From NPM

```sh
npm install --global @intelligence-ai/jsonschema
```

### From GitHub Releases

We publish precompiled binaries for every supported platform to [GitHub
Releases](https://github.com/Intelligence-AI/jsonschema/releases), including a
[continuous](https://github.com/Intelligence-AI/jsonschema/releases/tag/continuous)
that is updated on every commit from the main branch.

For convenience, we also provide a POSIX shell script capable of installing the
latest pre-built binaries, which you can run as follows:

```sh
/bin/sh -c "$(curl -fsSL https://raw.githubusercontent.com/intelligence-ai/jsonschema/main/install -H "Cache-Control: no-cache, no-store, must-revalidate")"
```

### From Dockerfile

You can compile the JSON Schema CLI inside a container and run it with Docker
as follows:

```sh
docker build -t jsonschema .
```

Then, you run the JSON Schema CLI by mounting the desired directory as
`/workspace` as follows:

```sh
docker run -it -v "$PWD:/workspace" jsonschema lint --verbose myschema.json

# You can optionally add this to your .alias (or similar) file:
# alias jsonschema="docker run -it -v \"$PWD:/workspace\" jsonschema"
```

### Building from source

```sh
git clone https://github.com/intelligence-ai/jsonschema
cd jsonschema
cmake -S . -B ./build -DCMAKE_BUILD_TYPE:STRING=Release
cmake --build ./build --config Release --parallel 4
cmake --install ./build --prefix <prefix> \
  --config Release --verbose --component intelligence_jsonschema
```

Where `<prefix>` can be any destination prefix of your choosing, such as `/opt`
or `/usr/local`.

Usage
-----

The functionality provided by the JSON Schema CLI is divided into commands. The
following pages describe each command in detail. Additionally, running the JSON
Schema CLI without passing a command will print convenient reference
documentation:

- [Validating](./docs/validate.markdown)
- [Metaschema](./docs/metaschema.markdown) (ensure a schema is valid)
- [Testing](./docs/test.markdown)
- [Formatting](./docs/format.markdown)
- [Linting](./docs/lint.markdown)
- [Bundling](./docs/bundle.markdown) (for inlining remote references in a schema)
- [Framing](./docs/frame.markdown)

Coming Soon
-----------

This project is under heavy development, and we have a lot of cool things in
the oven. In the mean-time, star the project to show your support!

| Feature               | Description                                                                        | Status      |
|-----------------------|------------------------------------------------------------------------------------|-------------|
| Debugging             | Validate a JSON instance against a JSON Schema step by step, like LLDB and GDB     | Not started |
| Upgrading/Downgrading | Convert a JSON Schema into a later or older dialect                                | Not started |

License
-------

This project is governed by the [AGPL-3.0](./LICENSE) copyleft license.
