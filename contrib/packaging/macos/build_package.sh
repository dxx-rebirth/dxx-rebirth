#!/bin/bash
set -x

export MACOSX_DEPLOYMENT_TARGET=10.14

GIT_HASH=$(git rev-parse --short HEAD)

# Get utils
git clone https://github.com/auriamg/macdylibbundler.git
cd macdylibbundler
make
cd ..

find ./ -name '*.app'

build_app() {
    name="$1"
    prettyname="$2"
    
    zipfilename="${prettyname}-${GIT_HASH}.app.zip"
    
    cd build 
    
    # Bundle resources
    ../macdylibbundler/dylibbundler -od -b -x ${prettyname}.app/Contents/MacOS/${name} -d ${prettyname}.app/Contents/libs
        
    # zip up and output to top level dir
    zip -r -X ../${zipfilename} ${prettyname}.app
    
    cd ..
}

build_app "d1x-rebirth" "D1X-Rebirth"
build_app "d2x-rebirth" "D2X-Rebirth"

# Clean up
rm -rf macdylibbundler
