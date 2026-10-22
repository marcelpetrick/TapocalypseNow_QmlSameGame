#!/usr/bin/env bash
# Wrap a self-contained Linux install (cmake --install with TAPOCALYPSE_DEPLOY_QT=ON) into a single AppImage.
#
#   tools/make_appimage.sh <install-dir> <output.AppImage> <appimagetool> <runtime>
#
# appimagetool and the type2 runtime are passed in (the release workflow pins and checksums both), so nothing is
# downloaded here. The install already carries Qt with RUNPATH $ORIGIN/../lib and a qt.conf, so the AppDir is that
# tree under usr/ plus the desktop entry and icon.
set -euo pipefail

if [[ $# -ne 4 ]]; then
    echo "usage: $0 <install-dir> <output.AppImage> <appimagetool> <runtime>" >&2
    exit 2
fi

repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
install_dir="$1"
output="$2"
appimagetool="$3"
runtime="$4"

[[ -x "${install_dir}/bin/tapocalypse" ]] || { echo "no bin/tapocalypse in ${install_dir}" >&2; exit 1; }

work="$(mktemp -d)"
trap 'rm -r -- "${work}"' EXIT
appdir="${work}/Tapocalypse.AppDir"

mkdir -p "${appdir}/usr"
cp -a "${install_dir}/." "${appdir}/usr/"
ln -s usr/bin/tapocalypse "${appdir}/AppRun"

install -Dm644 "${repo}/packaging/tapocalypse.desktop" "${appdir}/tapocalypse.desktop"
install -Dm644 "${repo}/packaging/tapocalypse.desktop" "${appdir}/usr/share/applications/tapocalypse.desktop"
install -Dm644 "${repo}/packaging/tapocalypse.svg" "${appdir}/tapocalypse.svg"
install -Dm644 "${repo}/packaging/tapocalypse.svg" "${appdir}/usr/share/icons/hicolor/scalable/apps/tapocalypse.svg"
ln -s tapocalypse.svg "${appdir}/.DirIcon"

# The runner may lack FUSE; extracting appimagetool instead of mounting it works everywhere. The extraction lands
# in TMPDIR, which is the work directory removed on exit.
TMPDIR="${work}" ARCH=x86_64 APPIMAGE_EXTRACT_AND_RUN=1 "${appimagetool}" --no-appstream --runtime-file "${runtime}" "${appdir}" "${output}"
chmod +x "${output}"
