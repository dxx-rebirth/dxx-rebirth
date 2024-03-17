#!/bin/bash

set -eux -o pipefail

# Grab latest AppImage package
curl	\
	--silent	\
	--show-error	\
	--location	\
	--output '#1'	\
	https://github.com/AppImage/AppImageKit/releases/download/continuous/'{appimagetool-x86_64.AppImage,AppRun-x86_64}'	\
	|| exit 3
chmod a+x appimagetool-x86_64.AppImage AppRun-x86_64

build_appimage() {
    name="$1"
    prettyname="$2"

    appdir="${name}.appdir"
    appimagename="${prettyname}.AppImage"

    # Install
    mkdir "${appdir}"

    # Copy resources into package dir
    mkdir -p "${appdir}/usr/bin"
    cp --link "build/${name}/${name}" "${appdir}/usr/bin"

    mkdir -p "${appdir}/usr/share/pixmaps"
    cp --link "${name}/${name}.xpm" "${appdir}/usr/share/pixmaps"
    cp --link "${name}/${name}.xpm" "${appdir}/"

    mkdir -p "${appdir}/usr/share/icons/hicolor/128x128/apps/"
    cp --link "${name}/${name}.png" "${appdir}/usr/share/icons/hicolor/128x128/apps/"
    cp --link "${name}/${name}.png" "${appdir}/"

    mkdir -p "${appdir}/usr/share/applications"
    cp --link "${name}/${name}.desktop" "${appdir}/usr/share/applications"
    cp --link "${name}/${name}.desktop" "${appdir}/"

    # Package
    cp --link "AppRun-x86_64" "${appdir}/AppRun"

    # Package!
    "./appimagetool-x86_64.AppImage" --no-appstream --verbose "${appdir}" "${appimagename}"

    # Clean
    rm -rf "${appdir}"
}

# Build each subunit
build_appimage "d1x-rebirth" "D1X-Rebirth"
build_appimage "d2x-rebirth" "D2X-Rebirth"

# Consolidate into a single zip file
zip -r -X DXX-Rebirth-Linux.zip D1X-Rebirth.AppImage D2X-Rebirth.AppImage

# Clean
rm -f appimagetool* AppRun* D1X-Rebirth.AppImage D2X-Rebirth.AppImage
