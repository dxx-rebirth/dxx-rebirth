#!/bin/bash
set -x

export MACOSX_DEPLOYMENT_TARGET=10.14

GIT_HASH=$(git rev-parse --short HEAD)


build_app() {
    name="$1"
    prettyname="$2"
    
    zipfilename="${prettyname}-${GIT_HASH}.app.zip"
    
    cd build 
    
    # Libs should already be bundled in with the `macos_bundle_libs=1` option to scons
        
    # zip up and output to top level dir
    zip -r -X ../${zipfilename} ${prettyname}.app
    
    cd ..
}

build_app "d1x-rebirth" "D1X-Rebirth"
build_app "d2x-rebirth" "D2X-Rebirth"

# Clean up
