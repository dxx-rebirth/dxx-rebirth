#!/bin/bash
set -x

GIT_HASH=$(git rev-parse --short HEAD)

# Grab latest AppImage package
curl -s -L -O https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage || exit 3
curl -s -L -O https://github.com/AppImage/AppImageKit/releases/download/continuous/AppRun-x86_64 || exit 3
chmod a+x appimagetool-x86_64.AppImage AppRun-x86_64

build_appimage() {
    name="$1"
    prettyname="$2"

    appdir="${name}.appdir"
    appimagename="${prettyname}-${GIT_HASH}.AppImage"

    # Install
    mkdir "${appdir}"

    # Copy resources into package dir
    mkdir -p "${appdir}/usr/bin"
    cp "build/${name}/${name}" "${appdir}/usr/bin"

    mkdir -p "${appdir}/usr/share/pixmaps"
    cp "${name}/${name}.xpm" "${appdir}/usr/share/pixmaps"
    cp "${name}/${name}.xpm" "${appdir}/"

    mkdir -p "${appdir}/usr/share/icons/hicolor/128x128/apps/"
    cp "${name}/${name}.png" "${appdir}/usr/share/icons/hicolor/128x128/apps/"
    cp "${name}/${name}.png" "${appdir}/"

    mkdir -p "${appdir}/usr/share/applications"
    cp "${name}/${name}.desktop" "${appdir}/usr/share/applications"
    cp "${name}/${name}.desktop" "${appdir}/"

    # Package
    cp "AppRun-x86_64" "${appdir}/AppRun"

    # Package!
    "./appimagetool-x86_64.AppImage" --no-appstream --verbose "${appdir}" "${appimagename}"

    # Generate .zsync file
    "./appimagetool-x86_64.AppImage" --generate-zsync "${appimagename}"

    # Clean
    rm -rf "${appdir}"
}

# Build each subunit
build_appimage "d1x-rebirth" "DXX-Rebirth"
build_appimage "d2x-rebirth" "DXX-Rebirth"

# Consolidate into a single zip file
zip -r -X DXX-Rebirth.zip D1X-Rebirth-${GIT_HASH}.AppImage D1X-Rebirth-${GIT_HASH}.AppImage.zsync D2X-Rebirth-${GIT_HASH}.AppImage D2X-Rebirth-${GIT_HASH}.AppImage.zsync

# Clean
rm -f appimagetool* AppRun* D1X-Rebirth-${GIT_HASH}.AppImage D1X-Rebirth-${GIT_HASH}.AppImage.zsync D2X-Rebirth-${GIT_HASH}.AppImage D2X-Rebirth-${GIT_HASH}.AppImage.zsync
