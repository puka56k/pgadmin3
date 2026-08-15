#!/usr/bin/env bash
# =============================================================================
# package_release.sh — assemble a portable Windows zip for pgAdmin3.
#
# This does NOT compile anything and does NOT require Windows or a Windows
# toolchain. `x64/Release/` already carries a pre-built `pgAdmin3.exe` plus
# its DLLs (committed by whoever last built on an actual Windows machine --
# see git log on `x64/Release/pgAdmin3.exe`); this script only *packages*
# that folder into a self-contained zip, on top of two gaps that folder has
# today:
#
#   1. Guru hint HTML docs (`app-docs/<lang>/hints/*.html` + `pgadmin3.css`)
#      -- x64/Release/ ships zero languages' worth of these today, even
#      though the app looks for them at runtime (see pgAdmin3.cpp's
#      DOC_DIR/LocatePath -- resolved relative to the exe's own directory,
#      same mechanism upstream pgadmin-org/pgadmin3 uses). Every language
#      folder under app-docs/ that has both a hints/ dir and pgadmin3.css is
#      copied in as docs/<lang>/.
#   2. Missing wxstd.mo (wx's own stock-string catalog) for 34 of the 45
#      shipped languages -- see i18n/wx-stock/README.md for where those come
#      from and why. Only fills in languages that don't already have one in
#      x64/Release/i18n/<lang>/.
#
# Nothing under x64/Release/ or app-docs/ is modified -- everything is
# assembled fresh in a staging directory (default: build-windows/dist/,
# gitignored) that's zipped up and thrown away layout-wise; only the
# resulting .zip (and its sha256) matter.
#
# Usage: windows/package_release.sh <version> [output-dir]
#   <version>    e.g. 2026.08.16 -- used only for the zip's filename.
#   [output-dir] where to write the zip + checksum (default: build-windows/dist)
# =============================================================================
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

if [ $# -lt 1 ]; then
	echo "Usage: $0 <version> [output-dir]" >&2
	exit 1
fi

VERSION="$1"
OUTPUT_DIR="${2:-build-windows/dist}"
PKG_NAME="pgAdmin3-${VERSION}-windows-x64"
ZIP_NAME="${PKG_NAME}.zip"
STAGE_ROOT="build-windows/stage"
STAGE_DIR="${STAGE_ROOT}/${PKG_NAME}"

if [ ! -d "x64/Release" ]; then
	echo "Error: x64/Release/ not found -- nothing to package." >&2
	exit 1
fi

echo "==> Staging Windows package in ${STAGE_DIR}..."
rm -rf "$STAGE_DIR"
mkdir -p "$STAGE_DIR"
cp -R "x64/Release/." "$STAGE_DIR/"

# ── Guru hints: copy every app-docs/<lang>/ that looks like a language pack
# (has both hints/ and pgadmin3.css) into docs/<lang>/ -- the layout
# pgAdmin3.cpp's DOC_DIR/LocatePath expects next to the exe. ──────────────
echo "==> Bundling guru hints..."
HINT_LANGS=0
for langdir in app-docs/*/; do
	lang="$(basename "$langdir")"
	if [ -d "${langdir}hints" ] && [ -f "${langdir}pgadmin3.css" ]; then
		mkdir -p "${STAGE_DIR}/docs/${lang}"
		cp -R "${langdir}hints" "${STAGE_DIR}/docs/${lang}/"
		cp "${langdir}pgadmin3.css" "${STAGE_DIR}/docs/${lang}/"
		HINT_LANGS=$((HINT_LANGS + 1))
	fi
done
echo "    ${HINT_LANGS} language(s) bundled."

# ── wxstd.mo: fill in only what's missing, from the vendored wx-stock/ set.
WX_STOCK_DIR="i18n/wx-stock"
if [ -d "$WX_STOCK_DIR" ]; then
	echo "==> Filling in missing wxstd.mo catalogs..."
	FILLED=0
	for langdir in "${STAGE_DIR}"/i18n/*/; do
		lang="$(basename "$langdir")"
		if [ ! -f "${langdir}wxstd.mo" ] && [ -f "${WX_STOCK_DIR}/${lang}/wxstd.mo" ]; then
			cp "${WX_STOCK_DIR}/${lang}/wxstd.mo" "${langdir}wxstd.mo"
			FILLED=$((FILLED + 1))
		fi
	done
	echo "    ${FILLED} language(s) filled in."
fi

# ── Report any pgadmin3.mo language that's still missing a wxstd.mo, so
# gaps stay visible instead of silently shipping incomplete. ────────────────
STILL_MISSING=()
for langdir in "${STAGE_DIR}"/i18n/*/; do
	lang="$(basename "$langdir")"
	[ -f "${langdir}wxstd.mo" ] || STILL_MISSING+=("$lang")
done
if [ "${#STILL_MISSING[@]}" -gt 0 ]; then
	echo "    Note: still no wxstd.mo for: ${STILL_MISSING[*]}"
fi

echo "==> Zipping ${ZIP_NAME}..."
mkdir -p "$OUTPUT_DIR"
ZIP_PATH="$(cd "$OUTPUT_DIR" && pwd)/${ZIP_NAME}"
rm -f "$ZIP_PATH"
(cd "$STAGE_ROOT" && zip -rq "$ZIP_PATH" "$PKG_NAME")

if command -v sha256sum >/dev/null 2>&1; then
	(cd "$OUTPUT_DIR" && sha256sum "$ZIP_NAME" > "${ZIP_NAME%.zip}.sha256")
else
	(cd "$OUTPUT_DIR" && shasum -a 256 "$ZIP_NAME" > "${ZIP_NAME%.zip}.sha256")
fi
SHA="$(awk '{print $1}' "${OUTPUT_DIR}/${ZIP_NAME%.zip}.sha256")"

echo ""
echo "Built: ${ZIP_PATH}"
echo "sha256: ${SHA}"
