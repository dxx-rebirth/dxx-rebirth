#!/usr/bin/env zsh

function dxx_codesign {
  # codesign parameters:
  # --timestamp: Request a signing timestamp from Apple's servers (required for notarization).
  # --options=runtime: Enable a hardened runtime (required for notarization).
  # --verbose: More verbose output for tracking status.
  # --force: Force overwriting existing ad hoc or identity-signed signatures.  Existing resources are likely to have at least one signature (likely ad hoc) to replace.
  # --sign: The signing identity to use to sign the resource.
  codesign --timestamp --options=runtime --verbose --force --sign "${signing_identity[2]}" "$@"
}

# Echo a message to stdout, and include the name of the script so that users
# can recognize the origin when this script is run as part of a larger build
# job.
function emsg {
	echo "$0: $@"
}

# Echo a message to stderr, and include the string `error:` for users to easily
# find it via text search in a log.
function eerror {
	echo "$0: error: $@" >&2
}

zmodload zsh/zutil
autoload is-at-least

if ! is-at-least 5.8 ${ZSH_VERSION}; then
  eerror "zsh 5.8 is required for the notarization script.  Please update to macOS 12 or higher, or install zsh 5.8 or higher and place it before the system zsh in your path."
  exit 1
fi

zparseopts -D -E -F - s:=signing_identity -signing-identity:=signing_identity a:=app_bundle_path -app-bundle-path:=app_bundle_path b:=binary_name -binary-name:=binary_name z:=zip_path -zip-path:=zip_path k:=notarization_keychain_profile -notarization-keychain-profile:=notarization_keychain_profile i:=apple_id -apple-id:=apple_id t:=team_id -team-id:=team_id || exit 1

end_opts=$@[(i)(--|-)]
set -- "${@[0,end_opts-1]}" "${@[end_opts+1,-1]}"

DXX_SIGN_HAS_ERROR=0

if [[ -z "${signing_identity}" ]]; then
  eerror "--signing-identity is required."
  DXX_SIGN_HAS_ERROR=1
fi

if [[ -z "${app_bundle_path}" ]]; then
  eerror "--app-bundle-path is required."
  DXX_SIGN_HAS_ERROR=1
elif [[ ! -d "${app_bundle_path[2]}" ]]; then
  eerror "App bundle specified in --app-bundle-path (${app_bundle_path[2]}) does not exist."
  DXX_SIGN_HAS_ERROR=1
fi

if [[ -z "${binary_name}" ]]; then
  eerror "--binary-name is required."
  DXX_SIGN_HAS_ERROR=1
elif [[ ! -f "${app_bundle_path[2]}/Contents/MacOS/${binary_name[2]}" ]]; then
  eerror "Binary specified in --binary-name (${binary_name[2]}) does not exist."
  DXX_SIGN_HAS_ERROR=1
fi

if [[ -z "${zip_path}" ]]; then
  eerror "--zip-path is required."
  DXX_SIGN_HAS_ERROR=1
fi

if [[ -z "${notarization_keychain_profile}" ]]; then
  if [[ -z "${apple_id}" || -z "${team_id}" ]]; then
    eerror "If --notarization-keychain-profile is not provided, then both --apple-id and --team-id must be provided."
    DXX_SIGN_HAS_ERROR=1
  fi
fi

if [[ DXX_SIGN_HAS_ERROR -ne 0 ]]; then
  # No message is needed here.  A message was printed in the path(s) that set DXX_SIGN_HAS_ERROR.
  exit 1
fi

if [[ -d "${app_bundle_path[2]}/Contents/libs" ]]; then
  DXX_DYLIB_PATH="${app_bundle_path[2]}/Contents/libs"
else
  # Unset this to prevent surprising results if the parent process set this in
  # the environment.
  unset DXX_DYLIB_PATH
fi

DXX_BINARY_PATH="${app_bundle_path[2]}/Contents/MacOS/${binary_name[2]}"

if [[ ! -z "${DXX_DYLIB_PATH}" ]]; then
  emsg "Signing dylibs at ${DXX_DYLIB_PATH} with identity ${signing_identity[2]} ..."

  dxx_codesign "${DXX_DYLIB_PATH}"/*.dylib
  if [[ $? -ne 0 ]]; then
    eerror "Failed to sign dylibs in ${DXX_DYLIB_PATH}"
    exit 1
  fi
fi

emsg "Signing application binary at ${DXX_BINARY_PATH} with identity ${signing_identity[2]} ..."

dxx_codesign "${DXX_BINARY_PATH}"
if [[ $? -ne 0 ]]; then
  eerror "Failed to sign application binary ${DXX_BINARY_PATH}"
  exit 1
fi

emsg "Signing app bundle at ${app_bundle_path[2]} with identity ${signing_identity[2]} ..."

dxx_codesign "${app_bundle_path[2]}"
if [[ $? -ne 0 ]]; then
  eerror "Failed to sign app bundle ${app_bundle_path[2]}"
  exit 1
fi

DXX_ZIP_NAME=${zip_path[2]##*/}

DXX_TMP_ZIP_PATH="${TMPDIR}${DXX_ZIP_NAME}"

emsg "Compressing ${app_bundle_path[2]} to temporary ZIP file at ${DXX_TMP_ZIP_PATH} ..."

if [[ -f "${DXX_TMP_ZIP_PATH}" ]]; then
  rm -f "${DXX_TMP_ZIP_PATH}"
fi

if [[ -f "${DXX_TMP_ZIP_PATH}" ]]; then
  eerror "Unable to remove existing temporary ZIP file at ${DXX_TMP_ZIP_PATH}"
  exit 1
fi

/usr/bin/ditto -c -k --keepParent "${app_bundle_path[2]}" "${DXX_TMP_ZIP_PATH}"

if [[ ! -f "${DXX_TMP_ZIP_PATH}" ]]; then
  eerror "Error compressing app bundle to ZIP file."
  exit 1
fi

# Note that the notarization process does NOT change the app bundle or the ZIP file.
# Instead, Apple reads the signature associated with the submitted resources and, if
# they don't detect any malicious code, create a ticket on their side that can be
# requested by a macOS client for the specific resource that was signed in order to
# validate that it went through the notarization process.  This ticket can also be
# stapled to the app bundle for offline validation by macOS clients which are not
# connected to the Internet at the time.  This stapling process happens further in
# the script.

emsg "Beginning notarization process.  This may take a few minutes."

if [[ -z "${notarization_keychain_profile}" ]]; then
  emsg "Using Apple ID '${apple_id[2]}' and Apple team ID '${team_id[2]}' for notarization credentials."
  if [[ ! -z "${DXX_NOTARIZATION_PASSWORD}" ]]; then
    emsg "Using password passed in via DXX_NOTARIZATION_PASSWORD environment variable."
    xcrun notarytool submit "${DXX_TMP_ZIP_PATH}" --apple-id "${apple_id[2]}" --team-id "${team_id[2]}" --password "${DXX_NOTARIZATION_PASSWORD}" --wait
    if [[ $? -ne 0 ]]; then
      eerror "Error notarizing application.  Check history for details."
      exit 1
    fi
  else
    emsg "Enter your application-specific password when prompted to submit for notarization."
    xcrun notarytool submit "${DXX_TMP_ZIP_PATH}" --apple-id "${apple_id[2]}" --team-id "${team_id[2]}" --wait
    if [[ $? -ne 0 ]]; then
      eerror "Error notarizing application.  Check history for details."
      exit 1
    fi
  fi
else
  emsg "Using Keychain item specified in '${notarization_keychain_profile[2]}' for notarization credentials."
  xcrun notarytool submit "${DXX_TMP_ZIP_PATH}" --keychain-profile "${notarization_keychain_profile[2]}" --wait
  if [[ $? -ne 0 ]]; then
    eerror "Error notarizing application.  Check history for details."
    exit 1
  fi
fi

emsg "Stapling ticket to app bundle ${app_bundle_path[2]} ..."

xcrun stapler staple "${app_bundle_path[2]}"
if [[ $? -ne 0 ]]; then
  eerror "Failed to staple ticket to app bundle ${app_bundle_path[2]}"
  exit 1
fi

rm -f "${DXX_TMP_ZIP_PATH}"

if [[ -f "${zip_path[2]}" ]]; then
  rm -f "${zip_path[2]}"
fi

if [[ -f "${zip_path[2]}" ]]; then
  eerror "Unable to remove existing target ZIP file at ${zip_path[2]}"
  exit 1
fi

emsg "Creating ${zip_path[2]} ..."

/usr/bin/ditto -c -k --keepParent "${app_bundle_path[2]}" "${zip_path[2]}"
