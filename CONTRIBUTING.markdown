Contributing
============

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

Releasing
---------

```sh
git checkout main

# Update the VERSION in CMakeLists.txt
vim CMakeLists.txt
# Update the version in action.yml
vim action.yml
# Update the version in the GitHub Actions example
vim README.markdown

git add CMakeLists.txt action.yml
git commit -m "vX.Y.Z"
git tag -a "vX.Y.Z" -m "vX.Y.Z"
git push
```

Then update https://github.com/sourcemeta/homebrew-apps.
