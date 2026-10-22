#!/usr/bin/env bash
# Regenerate the README media from the real QML: screenshots from the end-to-end layout checks and a gameplay GIF/MP4
# from a recorded demo game. Needs a built tree (default: build/) and ffmpeg.
#
#   tools/make_media.sh [build-dir]
set -euo pipefail

repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="${1:-${repo}/build}"
media="${repo}/media"
work="$(mktemp -d)"
trap 'rm -r -- "${work}"' EXIT
mkdir -p "${work}/shots" "${work}/frames"

export QT_QPA_PLATFORM=offscreen
TAPOCALYPSE_E2E_SCREENSHOT_DIR="${work}/shots" "${build}/EndToEndTests" layout_fitsEveryScreenShape legalClick_removesWholeGroupAndScoresSquare > /dev/null
TAPOCALYPSE_E2E_RECORD_DIR="${work}/frames" "${build}/EndToEndTests" recordDemo > /dev/null

command cp -f "${work}/shots/layout-1920x1080.png" "${media}/screenshot-desktop.png"
command cp -f "${work}/shots/layout-600x1000.png" "${media}/screenshot-portrait.png"
command cp -f "${work}/shots/layout-2560x720.png" "${media}/screenshot-ultrawide.png"

ffmpeg -v error -y -framerate 15 -i "${work}/frames/frame-%04d.png" \
    -vf "scale=960:-2:flags=lanczos" -c:v libx264 -pix_fmt yuv420p -crf 23 -movflags +faststart "${media}/gameplay.mp4"
ffmpeg -v error -y -framerate 15 -i "${work}/frames/frame-%04d.png" \
    -vf "fps=12,scale=720:-1:flags=lanczos,split[a][b];[a]palettegen=max_colors=96:stats_mode=diff[p];[b][p]paletteuse=dither=bayer:bayer_scale=4:diff_mode=rectangle" \
    "${media}/gameplay.gif"
ls -l "${media}"
