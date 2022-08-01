#!/bin/bash
set -x

GIT_HASH=$(git rev-parse --short HEAD)

build_app() {
    name="$1"
    prettyname="$2"
    
    zipfilename="${prettyname}-win-${GIT_HASH}.zip"
    outdir="${prettyname}"
    tmpdir="packagetemp"
    
    # Have to bundle in separate directory because of case-insensitivity clashes
    mkdir ${tmpdir}
    cd ${tmpdir}
    
    mkdir ${outdir}
    mkdir ${outdir}/Demos
    mkdir ${outdir}/Missions
    mkdir ${outdir}/Screenshots

    # Copy executable and libraris
    cp ../build/${name}/${name}.exe ${outdir}/
    
    # Copy in .dlls the old fashioned way. This assumes all the needed DLLs are 
    # ones from the mingw64 installation
    ldd ${outdir}/${name}.exe |grep mingw64 |sort |cut -d' ' -f3 |while read dll; do cp "${dll}" ${outdir}/; done

    # Copy in other resources
    cp ../${name}/*.ini ${outdir}/
    cp ../COPYING.txt ${outdir}/
    cp ../GPL-3.txt ${outdir}/
    cp ../README.md ${outdir}/
    cp ../INSTALL.markdown ${outdir}/
        
    # zip up and output to top level dir
    zip -r -X ../${zipfilename} ${outdir}
    
    cd ..
    
    rm -rf ${tmpdir}
}

build_app "d1x-rebirth" "D1X-Rebirth"
build_app "d2x-rebirth" "D2X-Rebirth"

# Clean up

