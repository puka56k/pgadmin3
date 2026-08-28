#!/bin/bash
#
# Build a drag-to-install .dmg from the assembled pgAdmin III.app.
#
# macos/build_app.sh produces the bundle; this wraps it in the disk image macOS
# users expect: mount it, drag the app onto the Applications alias, eject. The
# release pipeline (macos/publish_release.sh) ships a .zip instead, which is
# what Homebrew wants -- this script is for handing someone a single file.
#
# Usage: macos/make_dmg.sh [BUILD_DIR]
#
#   VERSION   version string in the file name (default: today, matching the
#             date-based scheme publish_release.sh uses)
#
# Output: <BUILD_DIR>/dist/pgAdmin3-<version>-macos-arm64.dmg

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

BUILD_DIR="${1:-build-macos}"
APP_NAME="pgAdmin III.app"
APP_PATH="${BUILD_DIR}/${APP_NAME}"
VOL_NAME="pgAdmin III"
VERSION="${VERSION:-$(date +%Y.%m.%d)}"
DIST_DIR="${BUILD_DIR}/dist"
DMG_PATH="${DIST_DIR}/pgAdmin3-${VERSION}-macos-arm64.dmg"

if [ ! -d "$APP_PATH" ]; then
	echo "Error: $APP_PATH not found. Run 'make build' first." >&2
	exit 1
fi

# Refuse to ship a bundle that still reaches outside itself: it would launch
# here and die on any other Mac. build_app.sh is what makes this true, so this
# is a check that it actually ran, not a substitute for it.
if otool -L "${APP_PATH}/Contents/MacOS/pgAdmin3" | grep -qE '^\s+(/Users/|/opt/homebrew)'; then
	echo "Error: the bundle still links dylibs from outside itself:" >&2
	otool -L "${APP_PATH}/Contents/MacOS/pgAdmin3" | grep -E '^\s+(/Users/|/opt/homebrew)' >&2
	echo "Run macos/build_app.sh so the dependencies get copied in." >&2
	exit 1
fi

mkdir -p "$DIST_DIR"

STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT

echo "==> Staging ${APP_NAME} ..."
# -R to keep symlinks inside Contents/Frameworks intact; -p to preserve the
# executable bits and the code signature's file modes.
cp -Rp "$APP_PATH" "$STAGE/"

# The alias that makes the drag target obvious. A real symlink, not a Finder
# alias: hdiutil copies it verbatim and Finder renders it as the folder.
ln -s /Applications "$STAGE/Applications"

echo "==> Building ${DMG_PATH} ..."
rm -f "$DMG_PATH"
# UDZO: compressed and read-only, which is what a distributed image should be.
hdiutil create \
	-volname "$VOL_NAME" \
	-srcfolder "$STAGE" \
	-fs HFS+ \
	-format UDZO \
	-ov \
	-quiet \
	"$DMG_PATH"

# Ad-hoc signing only, as with the .app: enough for the machine that built it,
# and Gatekeeper on any other Mac will still want a right-click -> Open.
codesign --force --sign - "$DMG_PATH" 2>/dev/null || true

SIZE="$(du -h "$DMG_PATH" | cut -f1)"
echo ""
echo "Done: ${DMG_PATH} (${SIZE})"
echo "  Open it and drag \"${APP_NAME%.app}\" onto Applications."
