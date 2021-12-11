#!/bin/zsh

function array-has-value() {
  local testVariable=$1
  local keyValue=$2

  [[ $(eval "echo \${${testVariable}[$keyValue]+1}") == 1 ]] && return 0 || return 1
}

zmodload zsh/zutil
autoload is-at-least

if is-at-least 5.8 $ZSH_VERSION; then
  zparseopts -D -E -F - s:=signing_identity -signing-identity:=signing_identity a:=app_bundle_path -app-bundle-path:=app_bundle_path z:=zip_path -zip-path:=zip_path k:=notarization_keychain_profile -notarization-keychain-profile:=notarization_keychain_profile i:=apple_id -apple-id:=apple_id t:=team_id -team-id:=team_id p:=apple_password -apple-password:=apple_password || exit 1

  end_opts=$@[(i)(--|-)]
  set -- "${@[0,end_opts-1]}" "${@[end_opts+1,-1]}"
else
  zparseopts -D -E - s:=signing_identity -signing-identity:=signing_identity a:=app_bundle_path -app-bundle-path:=app_bundle_path z:=zip_path -zip-path:=zip_path k:=notarization_keychain_profile -notarization-keychain-profile:=notarization_keychain_profile i:=apple_id -apple-id:=apple_id t:=team_id -team-id:=team_id p:=apple_password -apple-password:=apple_password

  if (( $# )); then
    end_opts=$@[(i)(--|-)]
    if [[ -n ${invalid_opt::=${(M)@[0,end_opts-1]#-}} ]]; then
      echo >&2 "Invalid options: $invalid_opt"
      exit 1
    fi
    set -- "${@[0,end_opts-1]}" "${@[end_opts+1,-1]}"
  fi
fi

DXX_SIGN_HAS_ERROR=0

if [[ -z "$signing_identity" ]]; then
  echo "--signing-identity is required."
  DXX_SIGN_HAS_ERROR=1
fi

if [[ -z "$app_bundle_path" ]]; then
  echo "--app-bundle-path is required."
  DXX_SIGN_HAS_ERROR=1
elif [[ ! -d "$app_bundle_path[2]" ]]; then
  echo "App bundle specified in --app-bundle-path ($app_bundle_path[2]) does not exist."
  DXX_SIGN_HAS_ERROR=1
fi

if [[ -z "$zip_path" ]]; then
  echo "--zip-path is required."
  DXX_SIGN_HAS_ERROR=1
fi

if [[ -z "$notarization_keychain_profile" ]]; then
  if [[ -z "$apple_id" || -z "$team_id" ]]; then
    echo "If --notarization-keychain-profile is not provided, then --apple-id and --team-id must be provided."
    DXX_SIGN_HAS_ERROR=1
  fi
fi

if [[ DXX_SIGN_HAS_ERROR -ne 0 ]]; then
  exit 1
fi

echo "Signing $app_bundle_path..."

if [[ -d "${app_bundle_path[2]}/Contents/libs" ]]; then
  DXX_DYLIB_PATH="${app_bundle_path[2]}/Contents/libs"
fi

DXX_BINARY_PATH="${app_bundle_path[2]}/Contents/MacOS"

if [[ ! -z "$DXX_DYLIB_PATH" ]]; then
  echo "Signing dylibs..."

  codesign --timestamp --options=runtime --verbose --force --sign "$signing_identity[2]" "$DXX_DYLIB_PATH"/*.dylib
  if [[ $? -ne 0 ]]; then
    echo "Failed to sign dylibs."
    exit 1
  fi
fi

echo "Signing application binary..."

codesign --timestamp --options=runtime --verbose --force --sign "$signing_identity[2]" "$DXX_BINARY_PATH"/*
if [[ $? -ne 0 ]]; then
  echo "Failed to sign application binary."
  exit 1
fi

echo "Signing app bundle..."

codesign --timestamp --options=runtime --verbose --force --sign "$signing_identity[2]" "$app_bundle_path[2]"
if [[ $? -ne 0 ]]; then
  echo "Failed to sign app bundle."
  exit 1
fi

echo "Compressing $app_bundle_path[2] to temporary ZIP file..."

DXX_ZIP_NAME=${zip_path[2]##*/}

DXX_TMP_ZIP_PATH="$TMPDIR""$DXX_ZIP_NAME"

if [[ -f "$DXX_TMP_ZIP_PATH" ]]; then
  rm -f "$DXX_TMP_ZIP_PATH"
fi

/usr/bin/ditto -c -k --keepParent "$app_bundle_path[2]" "$DXX_TMP_ZIP_PATH"

if [[ ! -f "$DXX_TMP_ZIP_PATH" ]]; then
  echo "Error compressing app bundle to ZIP file."
  exit 1
fi

echo "Beginning notarization process.  This may take a few minutes."

if [[ -z "$notarization_keychain_profile" ]]; then
  if [[ ! -z "$apple_password" ]]; then
    xcrun notarytool submit "$DXX_TMP_ZIP_PATH" --apple-id "$apple_id[2]" --team-id "$team_id[2]" --password "$apple_password[2]" --wait
    if [[ $? -ne 0 ]]; then
      echo "Error notarizing application.  Check history for details."
      exit 1
    fi
  else
    echo "Enter your application-specific password when prompted to submit for notarization."
    xcrun notarytool submit "$DXX_TMP_ZIP_PATH" --apple-id "$apple_id[2]" --team-id "$team_id[2]" --wait
    if [[ $? -ne 0 ]]; then
      echo "Error notarizing application.  Check history for details."
      exit 1
    fi
  fi
else
  xcrun notarytool submit "$DXX_TMP_ZIP_PATH" --keychain-profile "$notarization_keychain_profile[2]" --wait
  if [[ $? -ne 0 ]]; then
    echo "Error notarizing application.  Check history for details."
    exit 1
  fi
fi

xcrun stapler staple "$app_bundle_path[2]"
if [[ $? -ne 0 ]]; then
  echo "Failed to staple ticket to app bundle."
  exit 1
fi

rm -f "$DXX_TMP_ZIP_PATH"

if [[ -f "$zip_path[2]" ]]; then
  rm -f "$zip_path[2]"
fi

/usr/bin/ditto -c -k --keepParent "$app_bundle_path[2]" "$zip_path[2]"
