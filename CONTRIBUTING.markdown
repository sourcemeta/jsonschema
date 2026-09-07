Contributing
============

> Also see the [Sourcemeta organization-wide contribution
> guidelines](https://github.com/sourcemeta/.github/blob/main/CONTRIBUTING.md)
> for general policies that apply across all our projects.

Thanks for your interest in contributing to this project! We tried to make the
contribution experience as simple as possible. This project can be built
without requiring external dependencies other than C++20 compile and CMake as
follows:

```sh
# On UNIX based systems
make

# On Windows (from a Visual Studio Developer Prompt)
nmake
```

The default target will build the project and run the test suite. You will also
find the CLI binary under `build/`. The specific location varies depending on
your CMake default generator.
