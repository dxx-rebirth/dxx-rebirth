#!/bin/bash
set -x

GIT_HASH=$(git rev-parse --short HEAD)

#ARCH=x86_64


# Grab latest AppImage package
curl -s -L -O https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage || exit 3
chmod a+x appimagetool-x86_64.AppImage

# And the AppRun
curl -s -L -O https://github.com/AppImage/AppImageKit/releases/download/continuous/AppRun-x86_64 || exit 3
chmod a+x AppRun-x86_64


build_appimage() {
    name="$1"
    prettyname="$2"

    appdir="${name}.appdir"
    appimagename="${prettyname}-${GIT_HASH}.AppImage"

    ## Install
    # Copy resources into package dir
    mkdir "${appdir}"

    # Executable
    mkdir -p ${appdir}/usr/bin
    cp build/${name}/${name} ${appdir}/usr/bin

    # Icons
    mkdir -p ${appdir}/usr/share/pixmaps
    cp ${name}/${name}.xpm ${appdir}/usr/share/pixmaps
    cp ${name}/${name}.xpm ${appdir}/

    mkdir -p ${appdir}/usr/share/icons/hicolor/128x128/apps/
    cp ${name}/${name}.png ${appdir}/usr/share/icons/hicolor/128x128/apps/
    cp ${name}/${name}.png ${appdir}/

    # Menu item
    mkdir -p ${appdir}/usr/share/applications
    cp ${name}/${name}.desktop ${appdir}/usr/share/applications
    cp ${name}/${name}.desktop ${appdir}/

    ## Package
    cp AppRun-x86_64 ${appdir}/AppRun

    # Package!
    ./appimagetool-x86_64.AppImage --no-appstream --verbose "${appdir}" "${appimagename}"

    rm -rf ${appdir}
}

# Build each subunit
build_appimage "d1x-rebirth" "d1x-rebirth"
build_appimage "d2x-rebirth" "d2x-rebirth"

# Clean
rm -f appimagetool* AppRun*
