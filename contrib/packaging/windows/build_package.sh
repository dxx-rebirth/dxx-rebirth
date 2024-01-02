#!/bin/bash
set -x

GIT_HASH=$(git rev-parse --short HEAD)

build_app() {
    name="$1"
    prettyname="$2"
    
    zipfilename="${prettyname}-win.zip"
    outdir="."

    # Create a subdirectory for each app at the top level
    mkdir -p ${outdir}/${prettyname}/Demos
    mkdir -p ${outdir}/${prettyname}/Missions
    mkdir -p ${outdir}/${prettyname}/Screenshots

    # Copy executable and libraries to the respective app directory
    cp build/${name}/${name}.exe ${outdir}/${prettyname}/

    # Copy DLLs
    ldd ${outdir}/${prettyname}/${name}.exe | grep mingw64 | sort | cut -d' ' -f3 | while read dll; do cp "${dll}" ${outdir}/${prettyname}/; done

    # Copy other resources to the respective app directory
    cp ${name}/*.ini ${outdir}/${prettyname}/
    cp COPYING.txt ${outdir}/${prettyname}/
    cp GPL-3.txt ${outdir}/${prettyname}/
    cp README.md ${outdir}/${prettyname}/
    cp INSTALL.markdown ${outdir}/${prettyname}/
}

# Build D1X-Rebirth
build_app "d1x-rebirth" "D1X-Rebirth"

# Build D2X-Rebirth
build_app "d2x-rebirth" "D2X-Rebirth"

# zip up and output to top-level dir
zip -r -X dxx-rebirth-win.zip D1X-Rebirth D2X-Rebirth

# Clean up
rm -rf D1X-Rebirth D2X-Rebirth
