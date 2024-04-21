#!/bin/bash
set -eux -o pipefail

arch=$(uname -m)
outdir="./tmp"

build_app() {
    name="$1"
    prettyname="$2"
    
    # Create a subdirectory for each app at the top level
    mkdir -p "${outdir}/${prettyname}/Demos"
    mkdir -p "${outdir}/${prettyname}/Missions"
    mkdir -p "${outdir}/${prettyname}/Screenshots"

    # Copy executable to the respective app directory
    cp --link "build/${name}/${name}.exe" "${outdir}/${prettyname}/"

    # Copy libraries to the respective app directory
    ldd "${outdir}/${prettyname}/${name}.exe" | grep mingw64 | sort | cut -d' ' -f3 | while read dll; do cp "${dll}" "${outdir}/${prettyname}/"; done

    # Copy other resources to the respective app directory
    cp --link "${name}/"*.ini "${outdir}/${prettyname}/"
    cp --link "COPYING.txt" "${outdir}/${prettyname}/"
    cp --link "GPL-3.txt" "${outdir}/${prettyname}/"
    cp --link "README.md" "${outdir}/${prettyname}/"
    cp --link "INSTALL.markdown" "${outdir}/${prettyname}/"
}

# Build each app
build_app "d1x-rebirth" "D1X-Rebirth"
build_app "d2x-rebirth" "D2X-Rebirth"

# Consolidate both apps into a single zip file
cd "${outdir}"
zip -r -X "../DXX-Rebirth-Win-${arch}.zip" "D1X-Rebirth" "D2X-Rebirth"
