#!/bin/bash
# Create a Slackware package for sbbdep
# Usage: ./slackbuild/create-pkg.sh
# Output: tmp/sbbdep-VERSION-ARCH-BUILD.tgz

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRCDIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# Extract version from CMakeLists.txt
MAJOR=$(grep 'set(sbbdep_MAJOR_VERSION' "$SRCDIR/CMakeLists.txt" | grep -o '[0-9]*')
MINOR=$(grep 'set(sbbdep_MINOR_VERSION' "$SRCDIR/CMakeLists.txt" | grep -o '[0-9]*')
PATCH=$(grep 'set(sbbdep_PATCH_VERSION' "$SRCDIR/CMakeLists.txt" | grep -o '[0-9]*')
VERSION="${MAJOR}.${MINOR}.${PATCH}"

ARCH="${ARCH:-$(uname -m)}"
BUILD="${BUILD:-1}"
PKGNAME="sbbdep-${VERSION}-${ARCH}-${BUILD}"

PKG="$SRCDIR/tmp/install"
OUTPUT="$SRCDIR/tmp"

echo "Building sbbdep $VERSION ($ARCH) ..."

# Build
cd "$SRCDIR"
cmake --workflow --preset release

# Clean staging area
rm -rf "$PKG"
mkdir -p "$PKG"

# Install into staging area
DESTDIR="$PKG" cmake --install build/ninja \
	--config Release \
	--prefix /usr \
	--component sbbdep

# gzip the man page as Slackware expects
find "$PKG/usr/man" -name "*.1" 2>/dev/null | while read f; do
	gzip -9 "$f"
done

# Create the install directory with slack-desc
mkdir -p "$PKG/install"
cp "$SCRIPT_DIR/slack-desc" "$PKG/install/slack-desc"

# Build the package
mkdir -p "$OUTPUT"
cd "$PKG"
/sbin/makepkg -l y -c n "$OUTPUT/${PKGNAME}.tgz"

echo "Package created: $OUTPUT/${PKGNAME}.tgz"
