#!/bin/bash
set -eux -o pipefail

arch=$(uname -m)

# Grab latest AppImage package
curl --silent --show-error --location --output "#1" \
    https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/"{linuxdeploy-x86_64.AppImage}" || exit 3
chmod a+x linuxdeploy-x86_64.AppImage

build_appimage() {
    name="$1"
    prettyname="$2"

    # Package!
    export OUTPUT="${prettyname}.AppImage"
    ./linuxdeploy-x86_64.AppImage \
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
