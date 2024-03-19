#!/bin/bash
set -eux -o pipefail

arch=$(uname -m)

# Consolidate both apps into a single zip file
cd "./build"
zip -r -X "../DXX-Rebirth-MacOS${MACOSX_DEPLOYMENT_TARGET}-${arch}.zip" "D1X-Rebirth.app" "D2X-Rebirth.app"
