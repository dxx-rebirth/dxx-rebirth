#!/bin/bash
set -eux -o pipefail

arch=$(uname -m)
appimage="linuxdeploy-$arch.AppImage"

# Grab AppImage package at specific version
curl \
	--fail \
    --silent \
    --show-error \
    --location \
    --output "#1" \
	https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20240109-1/"{$appimage}" \
    || exit 3
chmod a+x "$appimage"

build_appimage() {
    name="$1"
    prettyname="$2"

    # Package!
    OUTPUT="${prettyname}.AppImage"	\
    "./$appimage" \
        --output appimage \
        --appdir="${name}.appdir" \
        --executable="build/${name}/${name}" \
        --desktop-file="${name}/${name}.desktop" \
        --icon-file="${name}/${name}.png"
}

# Build each app
build_appimage "d1x-rebirth" "D1X-Rebirth"
build_appimage "d2x-rebirth" "D2X-Rebirth"
