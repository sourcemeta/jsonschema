#!/bin/sh

set -o errexit
set -o nounset

# Because mpdecimal does not host the code in any Git provider
# See https://www.bytereef.org/mpdecimal/index.html

VERSION="4.0.1"
# See https://www.bytereef.org/mpdecimal/download.html
URL="https://www.bytereef.org/software/mpdecimal/releases/mpdecimal-$VERSION.tar.gz"

DIRECTORY="./vendor/mpdecimal"
rm -rf "$DIRECTORY"
mkdir -p "$DIRECTORY"
curl --location --progress-bar "$URL" | tar -xz --strip-components=1 -C "$DIRECTORY"

# Masking
rm -rf "$DIRECTORY/doc"
rm -rf "$DIRECTORY/libmpdec/.objs"
rm -rf "$DIRECTORY/libmpdec/.pc"
rm -rf "$DIRECTORY/libmpdec/.profile"
rm -rf "$DIRECTORY/libmpdec/examples"
rm -rf "$DIRECTORY/libmpdec/literature"
rm -rf "$DIRECTORY/libmpdec/Makefile.in"
rm -rf "$DIRECTORY/libmpdec/Makefile.vc"
rm -rf "$DIRECTORY/libmpdec/README.txt"
rm -rf "$DIRECTORY/libmpdec/bench.c"
rm -rf "$DIRECTORY/libmpdec/bench_full.c"
rm -rf "$DIRECTORY/libmpdec++"
rm -rf "$DIRECTORY/tests"
rm -rf "$DIRECTORY/tests++"
rm -rf "$DIRECTORY/vcbuild"
rm -rf "$DIRECTORY/CHANGELOG.txt"
rm -rf "$DIRECTORY/config.guess"
rm -rf "$DIRECTORY/config.h.in"
rm -rf "$DIRECTORY/config.sub"
rm -rf "$DIRECTORY/configure"
rm -rf "$DIRECTORY/configure.ac"
rm -rf "$DIRECTORY/install-sh"
rm -rf "$DIRECTORY/INSTALL.txt"
rm -rf "$DIRECTORY/Makefile.in"
rm -rf "$DIRECTORY/README.txt"
