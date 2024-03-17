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
    cp "build/${name}/${name}.exe" "${outdir}/tmp/${prettyname}/"

    # Copy DLLs
    ldd "${outdir}/tmp/${prettyname}/${name}.exe" | grep mingw64 | sort | cut -d' ' -f3 | while read dll; do cp "${dll}" "${outdir}/tmp/${prettyname}/"; done

    # Copy other resources to the respective app directory
    cp "${name}/"*.ini "${outdir}/tmp/${prettyname}/"
    cp COPYING.txt "${outdir}/tmp/${prettyname}/"
    cp GPL-3.txt "${outdir}/tmp/${prettyname}/"
    cp README.md "${outdir}/tmp/${prettyname}/"
    cp INSTALL.markdown "${outdir}/tmp/${prettyname}/"
}

# Build D1X-Rebirth
build_app "d1x-rebirth" "D1X-Rebirth"

# Build D2X-Rebirth
build_app "d2x-rebirth" "D2X-Rebirth"

# zip up and output to top-level dir
cd tmp
zip -r -X ../DXX-Rebirth-Win.zip D1X-Rebirth D2X-Rebirth
cd ..
