#!/bin/sh

set -o errexit
set -o nounset

if [ $# -lt 1 ]
then
  echo "Usage: $0 <version>" 1>&2
  exit 1
fi

OUTPUT="$(pwd)/build"
VERSION="$(echo "$1" | tr -d 'v')"

# (1) Download artifacts
mkdir -p "$OUTPUT/pypi/artifacts"
echo "Preparing $VERSION" 1>&2
PACKAGE_BASE_URL="https://github.com/sourcemeta/jsonschema/releases/download/v$VERSION"
curl --retry 5 --location --output "$OUTPUT/pypi/artifacts/darwin-arm64.zip" \
  "$PACKAGE_BASE_URL/jsonschema-$VERSION-darwin-arm64.zip"
curl --retry 5 --location --output "$OUTPUT/pypi/artifacts/darwin-x86_64.zip" \
  "$PACKAGE_BASE_URL/jsonschema-$VERSION-darwin-x86_64.zip"
curl --retry 5 --location --output "$OUTPUT/pypi/artifacts/linux-x86_64.zip" \
  "$PACKAGE_BASE_URL/jsonschema-$VERSION-linux-x86_64.zip"
curl --retry 5 --location --output "$OUTPUT/pypi/artifacts/linux-arm64.zip" \
  "$PACKAGE_BASE_URL/jsonschema-$VERSION-linux-arm64.zip"
curl --retry 5 --location --output "$OUTPUT/pypi/artifacts/windows-x86_64.zip" \
  "$PACKAGE_BASE_URL/jsonschema-$VERSION-windows-x86_64.zip"
unzip -o "$OUTPUT/pypi/artifacts/darwin-arm64.zip" -d "$OUTPUT/pypi/artifacts"
unzip -o "$OUTPUT/pypi/artifacts/darwin-x86_64.zip" -d "$OUTPUT/pypi/artifacts"
unzip -o "$OUTPUT/pypi/artifacts/linux-x86_64.zip" -d "$OUTPUT/pypi/artifacts"
unzip -o "$OUTPUT/pypi/artifacts/linux-arm64.zip" -d "$OUTPUT/pypi/artifacts"
unzip -o "$OUTPUT/pypi/artifacts/windows-x86_64.zip" -d "$OUTPUT/pypi/artifacts"
ls -l "$OUTPUT/pypi/artifacts"

# (2) Stage package contents
PKG="sourcemeta_jsonschema"
STAGE="$OUTPUT/pypi/staging/$PKG"
rm -rf "$STAGE"
mkdir -p "$STAGE/$PKG"

install -m 0755 "$OUTPUT/pypi/artifacts/jsonschema-$VERSION-darwin-arm64/bin/jsonschema" "$STAGE/$PKG/jsonschema-darwin-arm64"
install -m 0755 "$OUTPUT/pypi/artifacts/jsonschema-$VERSION-darwin-x86_64/bin/jsonschema" "$STAGE/$PKG/jsonschema-darwin-x86_64"
install -m 0755 "$OUTPUT/pypi/artifacts/jsonschema-$VERSION-linux-x86_64/bin/jsonschema" "$STAGE/$PKG/jsonschema-linux-x86_64"
install -m 0755 "$OUTPUT/pypi/artifacts/jsonschema-$VERSION-linux-arm64/bin/jsonschema" "$STAGE/$PKG/jsonschema-linux-arm64"
install -m 0755 "$OUTPUT/pypi/artifacts/jsonschema-$VERSION-windows-x86_64/bin/jsonschema.exe" "$STAGE/$PKG/jsonschema-windows-x86_64.exe"

cat > "$STAGE/$PKG/__main__.py" << 'EOF'
import os, platform, subprocess, sys

def main():
    system = platform.system().lower()
    arch   = platform.machine().lower()
    key    = f"{system}-{arch}"
    fn     = f"jsonschema-{key}.exe" if system=="windows" else f"jsonschema-{key}"
    path   = os.path.join(os.path.dirname(__file__), fn)
    if not os.path.exists(path):
        print(f"Unsupported platform: {key}", file=sys.stderr)
        sys.exit(1)
    subprocess.run([path]+sys.argv[1:], check=True)

if __name__=="__main__":
    main()
EOF

# (3) Build the package
cp README.markdown "$STAGE/README.md"
touch "$STAGE/$PKG/__init__.py"
cat > "$STAGE/MANIFEST.in" << EOF
include README.md
recursive-include $PKG *.exe
recursive-include $PKG jsonschema-*
EOF
cat > "$STAGE/setup.py" << EOF
from setuptools import setup, find_packages

setup(
    name         = "$PKG",
    version      = "$VERSION",
    description  = "The CLI for working with JSON Schema. Covers formatting, linting, testing, and much more for both local development and CI/CD pipelines",
    author       = "Sourcemeta",
    author_email = "hello@sourcemeta.com",
    url          = "https://github.com/sourcemeta/jsonschema",
    license      = "AGPL-3.0",
    packages     = find_packages(),
    include_package_data = True,
    package_data = {
        "$PKG": ["*.exe", "jsonschema-*"]
    },
    python_requires = ">=3.7",
    entry_points = {
        "console_scripts": [
            "jsonschema = ${PKG}.__main__:main"
        ]
    }
)
EOF
cd "$STAGE"
python3 setup.py sdist bdist_wheel
cd -

# (4) Try the wheel before publishing it
TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT
python3 -m venv "$TMP"
# shellcheck source=/dev/null
. "$TMP/bin/activate"
pip install "$STAGE/dist/$PKG-$VERSION-py3-none-any.whl"
"$TMP/bin/jsonschema" version
"$TMP/bin/jsonschema" help

# (5) Publish the package
twine upload "$STAGE/dist/*"
