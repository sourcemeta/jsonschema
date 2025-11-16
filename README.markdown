![JSON Schema](./assets/banner.png)

[![GitHub Release](https://img.shields.io/github/v/release/sourcemeta/jsonschema)](https://github.com/sourcemeta/jsonschema/releases)
[![NPM Version](https://img.shields.io/npm/v/@sourcemeta/jsonschema)](https://www.npmjs.com/package/@sourcemeta/jsonschema)
[![NPM Downloads](https://img.shields.io/npm/dm/%40sourcemeta%2Fjsonschema)](https://www.npmjs.com/package/@sourcemeta/jsonschema)
[![PyPI Version](https://img.shields.io/pypi/v/sourcemeta-jsonschema.svg)](https://pypi.org/project/sourcemeta-jsonschema)
[![GitHub Actions](https://github.com/sourcemeta/jsonschema/actions/workflows/test.yml/badge.svg)](https://github.com/sourcemeta/jsonschema/actions)
[![GitHub contributors](https://img.shields.io/github/contributors/sourcemeta/jsonschema.svg)](https://github.com/sourcemeta/jsonschema/graphs/contributors/)
[![pre-commit](https://img.shields.io/badge/pre--commit-enabled-brightgreen?logo=pre-commit&logoColor=white)](https://github.com/sourcemeta/jsonschema/blob/main/.pre-commit-hooks.yaml)

[![Get it from the Snap Store](https://snapcraft.io/en/light/install.svg)](https://snapcraft.io/jsonschema)

The command-line tool for working with [JSON Schema](https://json-schema.org),
the world most popular schema language. It is a comprehensive solution for
maintaining **repositories of schemas** and ensuring their quality, both during
local development and when running on CI/CD pipelines. For example:

- Ensure your schemas are valid
- Debug unexpected schema validation results
- Unit test your schemas against valid and invalid instances
- Enforce consistent indentation and keyword ordering in your schema files
- Detect and fix common JSON Schema anti-patterns
- Inline external references for conveniently distributing your schemas

[**Check out the documentation to learn more**](#usage)

***

> [!TIP]
> Do you want to level up your JSON Schema skills? Check out
> [learnjsonschema.com](https://www.learnjsonschema.com), our growing JSON
> Schema documentation website, our [JSON Schema for
> OpenAPI](https://www.sourcemeta.com/courses/jsonschema-for-openapi) video
> course, and our O'Reilly book [Unifying Business, Data, and Code: Designing
> Data Products with JSON
> Schema](https://www.oreilly.com/library/view/unifying-business-data/9781098144999/).

***

![JSON Schema CLI Example](./assets/example.png)

Version support
---------------

We aim to fully support _every_ version of JSON Schema and combinations between them.

| Dialect             | Support                                               |
|---------------------|-------------------------------------------------------|
| JSON Schema 2020-12 | **Full**                                              |
| JSON Schema 2019-09 | **Full**                                              |
| JSON Schema Draft 7 | **Full**                                              |
| JSON Schema Draft 6 | **Full**                                              |
| JSON Schema Draft 4 | **Full**                                              |
| JSON Schema Draft 3 | Partial (except `validate`, `test`, and `metaschema`) |
| JSON Schema Draft 2 | Partial (except `validate`, `test`, and `metaschema`) |
| JSON Schema Draft 1 | Partial (except `validate`, `test`, and `metaschema`) |
| JSON Schema Draft 0 | Partial (except `validate`, `test`, and `metaschema`) |

What our users are saying
-------------------------

> Amazing product. Very useful for formatting and bundling my schemas, plus it
> surfaced various referencing issues. 10 out of 10!

[@alombarte](https://github.com/alombarte), co-founder of the
[KrakenD](https://www.krakend.io) API Gateway.

Usage
-----

The functionality provided by the JSON Schema CLI is divided into commands. The
following pages describe each feature in detail. Additionally, running the JSON
Schema CLI without passing a command will print convenient reference
documentation:

- [`jsonschema version`](./docs/version.markdown)
- [`jsonschema validate`](./docs/validate.markdown)
- [`jsonschema metaschema`](./docs/metaschema.markdown) (ensure a schema is valid)
- [`jsonschema compile`](./docs/compile.markdown) (for pre-compiling schemas)
- [`jsonschema test`](./docs/test.markdown) (write unit tests for your schemas)
- [`jsonschema fmt`](./docs/format.markdown)
- [`jsonschema lint`](./docs/lint.markdown)
- [`jsonschema bundle`](./docs/bundle.markdown) (for inlining remote references in a schema)
- [`jsonschema inspect`](./docs/inspect.markdown) (for debugging references)
- [`jsonschema encode`](./docs/encode.markdown) (for binary compression)
- [`jsonschema decode`](./docs/decode.markdown)

> See [`jsonschema.json`](./docs/configuration.markdown) for an _experimental_
manifest for describing JSON Schema data models inspired by NPM's
`package.json`.

Note that YAML is supported in most commands!

We also support a growing amount of [`pre-commit`](https://pre-commit.com)
hooks. Take a look at the
[`.pre-commit-hooks.yaml`](https://github.com/sourcemeta/jsonschema/blob/main/.pre-commit-hooks.yaml)
configuration file for what's available right now. Keep in mind that to use the
`pre-commit` hooks, you need to install the CLI first.

***

If you are looking for more of a tutorial and overview of what the CLI is
capable of, take a look at the [Applying software engineering practices to JSON
Schemas](https://www.youtube.com/watch?v=wJ7bK22n3IU) talk from the 2024 [JSON
Schema Conference](https://conference.json-schema.org). It covers advise on
ontology design, linting, unit testing, CI/CD integration, and more:

[![JSON Schema Conference 2024 - Applying software engineering practices to JSON Schemas](https://img.youtube.com/vi/wJ7bK22n3IU/0.jpg)](https://www.youtube.com/watch?v=wJ7bK22n3IU)

Installation
------------

The JSON Schema CLI is written using C++ and [CMake](https://cmake.org/), and
supports macOS, Windows, and GNU/Linux.

### From Homebrew

```sh
brew install sourcemeta/apps/jsonschema
```

### From GitHub Actions

```yaml
- uses: sourcemeta/jsonschema@vX.Y.Z
```

Where `X.Y.Z` is replaced with the desired version. For example:

```yaml
- name: Checkout Repository
  uses: actions/checkout@v4

- name: Install the JSON Schema CLI
  uses: sourcemeta/jsonschema@v12.3.0

# Then use as usual
- run: jsonschema fmt path/to/schemas --check
```

### From npm

```sh
npm install --global @sourcemeta/jsonschema
```

### From PyPI

```sh
pip install sourcemeta-jsonschema
```

### From mise

```sh
mise use jsonschema
```

### From GitHub Releases

We publish precompiled binaries for every supported platform to [GitHub
Releases](https://github.com/sourcemeta/jsonschema/releases), including a
[continuous](https://github.com/sourcemeta/jsonschema/releases/tag/continuous)
that is updated on every commit from the main branch.

For convenience, we also provide a POSIX shell script capable of installing the
latest pre-built binaries, which you can run as follows:

```sh
curl -fsSL https://raw.githubusercontent.com/sourcemeta/jsonschema/main/install -H 'Cache-Control: no-cache, no-store, must-revalidate' | /bin/sh
```

Keep in mind that it is hard to provide binaries that work across GNU/Linux
distributions, given they often have major differences such as C runtimes (GLIC
vs MUSL). We conservatively target Ubuntu 22.04, but you might need to build
from source if your distribution of choice is different.

To verify the GPG signature of the checksums file:

```sh
curl --silent --show-error --location 'https://www.sourcemeta.com/gpg.asc' | gpg --import
gpg --verify CHECKSUMS.txt.asc CHECKSUMS.txt
```

### From Dockerfile

Starting from v7.2.1, we publish a Docker image to [GitHub
Packages](https://github.com/sourcemeta/jsonschema/pkgs/container/jsonschema)
(`amd64` and `arm64`), which you can use as follows:

```sh
docker run --interactive --volume "$PWD:/workspace" \
  ghcr.io/sourcemeta/jsonschema:vX.Y.Z lint --verbose myschema.json
```

Replace `vX.Y.Z` with your desired version. You can mount any directory as `/workspace`.

> [!WARNING]
> Make sure to NOT allocate a pseudo-TTY when running the CLI through Docker
> (i.e. the `--tty`/`-t` option) as it might result in line ending
> incompatibilities between the container and host, which will affect
> formatting. Plus a TTY is not required for running a tool like the JSON
> Schema CLI.

### From Snap

Starting from v10.0.0, we publish to the Snap store:

```sh
sudo snap install jsonschema
```

Keep in mind that due to [Snap
confinement](https://snapcraft.io/docs/snap-confinement) requirements, the Snap
is only able to access files under your `$HOME` directory.

### With gah

If you are using [gah](https://github.com/marverix/gah):

```sh
gah install jsonschema
```

gah does not require sudo, but you need to have `$HOME/.local/bin/` in your `PATH`.

### Building from source

```sh
git clone https://github.com/sourcemeta/jsonschema
cd jsonschema
cmake -S . -B ./build -DCMAKE_BUILD_TYPE:STRING=Release
cmake --build ./build --config Release --parallel 4
cmake --install ./build --prefix <prefix> \
  --config Release --verbose --component sourcemeta_jsonschema
```

Where `<prefix>` can be any destination prefix of your choosing, such as `/opt`
or `/usr/local`.
