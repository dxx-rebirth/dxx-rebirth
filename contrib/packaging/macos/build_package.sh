#!/bin/bash
set -x

export MACOSX_DEPLOYMENT_TARGET=10.14

GIT_HASH=$(git rev-parse --short HEAD)

build_app() {
    name="$1"
    prettyname="$2"
    
    cd build
        
    cd ..
}

# Build both applications
build_app "d1x-rebirth" "D1X-Rebirth"
build_app "d2x-rebirth" "D2X-Rebirth"

# Create a single zip file containing both applications
zip -r -X DXX-Rebirth.zip D1X-Rebirth.app D2X-Rebirth.app

# Clean up
rm -rf D1X-Rebirth.app D2X-Rebirth.app
