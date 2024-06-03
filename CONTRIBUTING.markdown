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
git add CMakeLists.txt
git commit -m "vX.Y.Z"
git tag -a "vX.Y.Z" -m "vX.Y.Z"
git push
```

Then update https://github.com/Intelligence-AI/homebrew-apps.

Grant of Rights
---------------

By contributing to this project, the Contributor irrevocably assigns,
transfers, and conveys to Intelligence.AI all right, title, and interest in
and to any contributions, including, but not limited to, all intellectual
property rights.

The contributor acknowledges and agrees that Intelligence.AI shall have the
exclusive right to use, reproduce, modify, distribute, publicly display,
publicly perform, sublicense, and otherwise exploit the contributions, whether
in original or modified form, for any purpose and without any obligation to
account to the contributor.

The contributor grants Intelligence.AI the right to re-license the
contributions, including any derivative works, under other licenses as
Intelligence.AI deems appropriate.
