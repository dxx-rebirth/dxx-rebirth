#!/bin/bash

set -eux -o pipefail

build_app() {
    name="$1"
    prettyname="$2"
    
    zipfilename="${prettyname}-win.zip"
    outdir="."

    # Create a subdirectory for each app at the top level
    mkdir -p "${outdir}/tmp/${prettyname}/Demos"
    mkdir -p "${outdir}/tmp/${prettyname}/Missions"
    mkdir -p "${outdir}/tmp/${prettyname}/Screenshots"

    # Copy executable and libraries to the respective app directory
    cp --link "build/${name}/${name}.exe" "${outdir}/tmp/${prettyname}/"

    # Copy DLLs
    ldd "${outdir}/tmp/${prettyname}/${name}.exe" | grep mingw64 | sort | cut -d' ' -f3 | while read dll; do cp "${dll}" "${outdir}/tmp/${prettyname}/"; done

    # Copy other resources to the respective app directory
    cp --link "${name}/"*.ini "${outdir}/tmp/${prettyname}/"
    cp --link COPYING.txt "${outdir}/tmp/${prettyname}/"
    cp --link GPL-3.txt "${outdir}/tmp/${prettyname}/"
    cp --link README.md "${outdir}/tmp/${prettyname}/"
    cp --link INSTALL.markdown "${outdir}/tmp/${prettyname}/"
}

# Build D1X-Rebirth
build_app "d1x-rebirth" "D1X-Rebirth"

# Build D2X-Rebirth
build_app "d2x-rebirth" "D2X-Rebirth"

# zip up and output to top-level dir
cd tmp
zip -r -X ../DXX-Rebirth-Win-`uname -m`.zip D1X-Rebirth D2X-Rebirth
