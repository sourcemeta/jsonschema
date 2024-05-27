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

The JSON Schema CLI is written using C++ and [CMake](https://cmake.org/), and supports macOS, Windows,
and GNU/Linux. Under the hood, it relies on the powerful [JSON
Toolkit](https://github.com/sourcemeta/jsontoolkit) library.

### GitHub Releases

We publish precompiled binaries for every supported platforms to [GitHub
Releases](https://github.com/Intelligence-AI/jsonschema/releases), including a
[continuous](https://github.com/Intelligence-AI/jsonschema/releases/tag/continuous)
that is updated on every commit from the main branch.

### Building from source

```sh
git clone https://github.com/intelligence-ai/jsonschema
cmake -S . -B ./build -DCMAKE_BUILD_TYPE:STRING=Release
cmake --build ./build --config Release --parallel 4
cmake --install ./build --prefix <prefix> \
  --config Release --verbose --component intelligence_jsonschema
```

Where `<prefix>` can be any destination prefix of your choosing, such as `/opt`
or `/usr/local`.

Usage
-----

The JSON Schema CLI supports a growing amount of commands. The following pages
describe each command in details. Additionally, running the JSON Schema CLI
without passing a command will print convenient reference documentation:

- [Validating](./docs/validate.markdown)
- [Testing](./docs/test.markdown)
- [Formatting](./docs/format.markdown)
- [Linting](./docs/lint.markdown)
- [Bundling](./docs/bundle.markdown)
- [Framing](./docs/frame.markdown)

The following global options apply to all commands:

- `--verbose / -v`: Enable verbose output
- `--resolve / -r`: Import the given JSON Schema into the resolution context

Coming Soon
-----------

This project is under heavy development, and we have a lot of cool things in
the oven. In the mean-time, star the project to show your support!

| Feature               | Description                                                                        | Status      |
|-----------------------|------------------------------------------------------------------------------------|-------------|
| Annotating            | Validate a JSON instance against a JSON Schema with annotation extraction support  | Not started |
| Debugging             | Validate a JSON instance against a JSON Schema step by step, like LLDB and GDB     | Not started |
| Upgrading/Downgrading | Convert a JSON Schema into a later or older dialect                                | Not started |

License
-------

This project is governed by the [AGPL-3.0](./LICENSE) copyleft license.
