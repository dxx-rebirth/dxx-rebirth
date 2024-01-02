#!/bin/bash
set -x

export MACOSX_DEPLOYMENT_TARGET=10.14

GIT_HASH=$(git rev-parse --short HEAD)

build_app() {
    name="$1"
    prettyname="$2"
    
    zipfilename="${prettyname}.app.zip"
    
    # Create a subdirectory for each app at the top level
    mkdir -p ${prettyname}.app/${prettyname}

    # Move the application content to the respective app directory
    mv build/${prettyname}.app/* ${prettyname}.app/${prettyname}

    # zip up and output to top-level dir
    zip -r -X ${zipfilename} ${prettyname}.app
    
    # Clean up
    rm -rf ${prettyname}.app
}

# Build D1X-Rebirth
build_app "d1x-rebirth" "D1X-Rebirth"

# Build D2X-Rebirth
build_app "d2x-rebirth" "D2X-Rebirth"

# zip up and output to top-level dir
zip -r -X dxx-rebirth-macos.zip D1X-Rebirth D2X-Rebirth

# Clean up
rm -rf D1X-Rebirth D2X-Rebirth
