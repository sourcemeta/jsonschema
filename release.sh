#!/bin/sh

set -o errexit
set -o nounset

usage() {
  echo "Usage: $0 <major|minor|patch>" 1>&2
  exit 1
}

if [ "$#" -ne 1 ]; then
  usage
fi

BUMP_TYPE="$1"

CURRENT_VERSION="$(cat VERSION)"
MAJOR="$(echo "$CURRENT_VERSION" | cut -d . -f1)"
MINOR="$(echo "$CURRENT_VERSION" | cut -d . -f2)"
PATCH="$(echo "$CURRENT_VERSION" | cut -d . -f3)"

case "$BUMP_TYPE" in
  major)
    MAJOR=$((MAJOR + 1))
    MINOR=0
    PATCH=0
    ;;
  minor)
    MINOR=$((MINOR + 1))
    PATCH=0
    ;;
  patch)
    PATCH=$((PATCH + 1))
    ;;
  *)
    usage
    ;;
esac

NEW_VERSION="${MAJOR}.${MINOR}.${PATCH}"
echo "Bumping version: ${CURRENT_VERSION} -> ${NEW_VERSION}"

replace_version() {
  file="$1"
  if ! grep -q "$CURRENT_VERSION" "$file"
  then
    echo "Error: ${CURRENT_VERSION} not found in ${file}" 1>&2
    exit 1
  fi
  sed -i.bak "s/${CURRENT_VERSION}/${NEW_VERSION}/g" "$file"
  rm -f "${file}.bak"
}

replace_version VERSION
replace_version README.markdown
replace_version action.yml
replace_version package.json
replace_version package-lock.json

git add VERSION README.markdown action.yml package.json package-lock.json
git commit --signoff --message "v${NEW_VERSION}"
git tag --annotate "v${NEW_VERSION}" --message "v${NEW_VERSION}"
git log -1 --patch
