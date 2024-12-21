#!/bin/bash
set -eux -o pipefail

arch=$(uname -m)

# Grab AppImage package at specific version
curl \
	--fail \
    --silent \
    --show-error \
    --location \
    --output "#1" \
    https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20240109-1/"{linuxdeploy-$arch.AppImage}" \
    || exit 3
chmod a+x "linuxdeploy-$arch.AppImage"

build_appimage() {
    name="$1"
    prettyname="$2"

    # Package!
    export OUTPUT="${prettyname}.AppImage"
    "./linuxdeploy-$arch.AppImage" \
        --output appimage \
        --appdir="${name}.appdir" \
        --executable="build/${name}/${name}" \
        --desktop-file="${name}/${name}.desktop" \
        --icon-file="${name}/${name}.png"
}

# Build each app
build_appimage "d1x-rebirth" "D1X-Rebirth"
build_appimage "d2x-rebirth" "D2X-Rebirth"

# Consolidate both apps into a single zip file
zip -r -X "DXX-Rebirth-Linux-AppImage-${arch}.zip" "D1X-Rebirth.AppImage" "D2X-Rebirth.AppImage"
