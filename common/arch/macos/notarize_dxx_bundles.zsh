#!/usr/bin/env zsh

function dxx_codesign {
  # codesign parameters:
  # --timestamp: Request a signing tiemstamp from Apple's servers (required for notarization).
  # --options=runtime: Enable a hardened runtime (required for notarization).
  # --verbose: More verbose output for tracking status.
  # --force: Force overwriting existing ad hoc or identity-signed signatures.  Existing resources are likely to have at least one signature (likely ad hoc) to replace.
  # --sign: The signing idnetity to use to sign the resource.
  codesign --timestamp --options=runtime --verbose --force --sign "${signing_identity[2]}" "$@"
}

zmodload zsh/zutil
autoload is-at-least

if ! is-at-least 5.8 ${ZSH_VERSION}; then
  echo "zsh 5.8 is required for the notarization script.  Please update to macOS 12 or higher, or install zsh 5.8 or higher and place it before the system zsh in your path."
  exit 1
fi

zparseopts -D -E -F - s:=signing_identity -signing-identity:=signing_identity a:=app_bundle_path -app-bundle-path:=app_bundle_path b:=binary_name -binary-name:=binary_name z:=zip_path -zip-path:=zip_path k:=notarization_keychain_profile -notarization-keychain-profile:=notarization_keychain_profile i:=apple_id -apple-id:=apple_id t:=team_id -team-id:=team_id p:=apple_password -apple-password:=apple_password || exit 1

end_opts=$@[(i)(--|-)]
set -- "${@[0,end_opts-1]}" "${@[end_opts+1,-1]}"

DXX_SIGN_HAS_ERROR=0

if [[ -z "${signing_identity}" ]]; then
  echo "--signing-identity is required."
  DXX_SIGN_HAS_ERROR=1
fi

if [[ -z "${app_bundle_path}" ]]; then
  echo "--app-bundle-path is required."
  DXX_SIGN_HAS_ERROR=1
elif [[ ! -d "${app_bundle_path[2]}" ]]; then
  echo "App bundle specified in --app-bundle-path (${app_bundle_path[2]}) does not exist."
  DXX_SIGN_HAS_ERROR=1
fi

if [[ -z "${binary_name}" ]]; then
  echo "--binary-name is required"
  DXX_SIGN_HAS_ERROR=1
elif [[ ! -f "${app_bundle_path[2]}/Contents/MacOS/${binary_name[2]}" ]]; then
  echo "Binary specified in --binary-name (${binary_name[2]}) does not exist."
  DXX_SIGN_HAS_ERROR=1
fi

if [[ -z "${zip_path}" ]]; then
  echo "--zip-path is required."
  DXX_SIGN_HAS_ERROR=1
fi

if [[ -z "${notarization_keychain_profile}" ]]; then
  if [[ -z "${apple_id}" || -z "${team_id}" ]]; then
    echo "If --notarization-keychain-profile is not provided, then --apple-id and --team-id must be provided."
    DXX_SIGN_HAS_ERROR=1
  fi
fi

if [[ DXX_SIGN_HAS_ERROR -ne 0 ]]; then
  exit 1
fi

if [[ -d "${app_bundle_path[2]}/Contents/libs" ]]; then
  DXX_DYLIB_PATH="${app_bundle_path[2]}/Contents/libs"
else
  unset DXX_DYLIB_PATH
fi

DXX_BINARY_PATH="${app_bundle_path[2]}/Contents/MacOS/${binary_name[2]}"

if [[ ! -z "${DXX_DYLIB_PATH}" ]]; then
  echo "Signing dylibs at ${app_bundle_path[2]}/Contents/libs with identity ${signing_identity[2]} ..."

  dxx_codesign "${DXX_DYLIB_PATH}"/*.dylib
  if [[ $? -ne 0 ]]; then
    echo "Failed to sign dylibs."
    exit 1
  fi
fi

echo "Signing application binary at ${DXX_BINARY_PATH} with identity ${signing_identity[2]} ..."

dxx_codesign "${DXX_BINARY_PATH}"
if [[ $? -ne 0 ]]; then
  echo "Failed to sign application binary."
  exit 1
fi

echo "Signing app bundle at ${app_bundle_path[2]} with identity ${signing_identity[2]} ..."

dxx_codesign "${app_bundle_path[2]}"
if [[ $? -ne 0 ]]; then
  echo "Failed to sign app bundle."
  exit 1
fi

DXX_ZIP_NAME=${zip_path[2]##*/}

DXX_TMP_ZIP_PATH="${TMPDIR}${DXX_ZIP_NAME}"

echo "Compressing $app_bundle_path[2] to temporary ZIP file at ${DXX_TMP_ZIP_PATH} ..."

if [[ -f "${DXX_TMP_ZIP_PATH}" ]]; then
  rm -f "${DXX_TMP_ZIP_PATH}"
fi

if [[ -f "${DXX_TMP_ZIP_PATH}" ]]; then
  echo "Unable to remove existing temporary ZIP file at ${DXX_TMP_ZIP_PATH}"
  exit 1
fi

/usr/bin/ditto -c -k --keepParent "${app_bundle_path[2]}" "${DXX_TMP_ZIP_PATH}"

if [[ ! -f "${DXX_TMP_ZIP_PATH}" ]]; then
  echo "Error compressing app bundle to ZIP file."
  exit 1
fi

echo "Beginning notarization process.  This may take a few minutes."

if [[ -z "${notarization_keychain_profile}" ]]; then
  echo "Using Apple ID ${apple_id[2]} and Apple team ID ${team_id[2]} for notarization credentials."
  if [[ ! -z "${apple_password}" ]]; then
    echo "Using password passed in via CLI parameter."
    xcrun notarytool submit "${DXX_TMP_ZIP_PATH}" --apple-id "${apple_id[2]}" --team-id "${team_id[2]}" --password "${apple_password[2]}" --wait
    if [[ $? -ne 0 ]]; then
      echo "Error notarizing application.  Check history for details."
      exit 1
    fi
  else
    echo "Enter your application-specific password when prompted to submit for notarization."
    xcrun notarytool submit "${DXX_TMP_ZIP_PATH}" --apple-id "${apple_id[2]}" --team-id "${team_id[2]}" --wait
    if [[ $? -ne 0 ]]; then
      echo "Error notarizing application.  Check history for details."
      exit 1
    fi
  fi
else
  echo "Using Keychain item specified in ${notarization_keychain_profile[2]} for notarization credentials."
  xcrun notarytool submit "${DXX_TMP_ZIP_PATH}" --keychain-profile "${notarization_keychain_profile[2]}" --wait
  if [[ $? -ne 0 ]]; then
    echo "Error notarizing application.  Check history for details."
    exit 1
  fi
fi

echo "Stapling ticket to app bundle ..."

xcrun stapler staple "${app_bundle_path[2]}"
if [[ $? -ne 0 ]]; then
  echo "Failed to staple ticket to app bundle."
  exit 1
fi

rm -f "${DXX_TMP_ZIP_PATH}"

if [[ -f "${zip_path[2]}" ]]; then
  rm -f "${zip_path[2]}"
fi

if [[ -f "${zip_path[2]}" ]]; then
  echo "Unable to remove existing target ZIP file at ${zip_path[2]}"
  exit 1
fi

echo "Creating ${zip_path[2]} ..."

/usr/bin/ditto -c -k --keepParent "${app_bundle_path[2]}" "${zip_path[2]}"
