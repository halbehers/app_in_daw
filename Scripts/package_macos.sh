#!/usr/bin/env bash
# Packages the built AU + VST3 bundles into a single macOS installer .pkg.
#
# Invoked both by CMake/packaging.cmake (as part of `cmake --build --preset release`) and by
# .github/workflows/build_and_test.yml in CI, so there's exactly one place this logic lives.
#
# Required env vars: PRODUCT_NAME, BUNDLE_ID, VERSION, AU_PATH, VST3_PATH, OUTPUT_DIR
# Optional env vars (real signing/notarization instead of ad-hoc, if all of the relevant ones are set):
#   DEVELOPER_ID_APPLICATION, DEVELOPER_ID_INSTALLER,
#   NOTARIZATION_USERNAME, NOTARIZATION_PASSWORD, NOTARIZATION_TEAM_ID
#   KEYCHAIN_PATH (only needed in CI, where the Developer ID certs are imported into a dedicated
#   keychain rather than the default login keychain a local dev's certs would already be in)

set -euo pipefail

: "${PRODUCT_NAME:?}" "${BUNDLE_ID:?}" "${VERSION:?}" "${AU_PATH:?}" "${VST3_PATH:?}" "${OUTPUT_DIR:?}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PACKAGING_DIR="$SCRIPT_DIR/../Packaging/macos"

WORK_DIR="$(mktemp -d)"
trap 'rm -rf "$WORK_DIR"' EXIT

if [ -n "${DEVELOPER_ID_APPLICATION:-}" ]; then
    echo "Signing with Developer ID Application: $DEVELOPER_ID_APPLICATION"
    keychain_args=()
    [ -n "${KEYCHAIN_PATH:-}" ] && keychain_args=(--keychain "$KEYCHAIN_PATH")
    codesign --force "${keychain_args[@]}" -s "$DEVELOPER_ID_APPLICATION" -v "$AU_PATH" --deep --strict --options=runtime --timestamp
    codesign --force "${keychain_args[@]}" -s "$DEVELOPER_ID_APPLICATION" -v "$VST3_PATH" --deep --strict --options=runtime --timestamp
else
    echo "No DEVELOPER_ID_APPLICATION set - ad-hoc signing (fine for local testing, not for distribution)."
    codesign --force --deep --sign - "$AU_PATH"
    codesign --force --deep --sign - "$VST3_PATH"
fi

pkgbuild --identifier "$BUNDLE_ID.au.pkg" --version "$VERSION" --component "$AU_PATH" \
    --install-location "/Library/Audio/Plug-Ins/Components" "$WORK_DIR/$PRODUCT_NAME.au.pkg"
pkgbuild --identifier "$BUNDLE_ID.vst3.pkg" --version "$VERSION" --component "$VST3_PATH" \
    --install-location "/Library/Audio/Plug-Ins/VST3" "$WORK_DIR/$PRODUCT_NAME.vst3.pkg"

PRODUCT_NAME="$PRODUCT_NAME" BUNDLE_ID="$BUNDLE_ID" VERSION="$VERSION" \
    envsubst < "$PACKAGING_DIR/distribution.xml.template" > "$WORK_DIR/distribution.xml"

mkdir -p "$OUTPUT_DIR"
ARTIFACT="$OUTPUT_DIR/$PRODUCT_NAME-$VERSION-macOS.pkg"

if [ -n "${DEVELOPER_ID_INSTALLER:-}" ]; then
    echo "Signing installer with Developer ID Installer: $DEVELOPER_ID_INSTALLER"
    keychain_args=()
    [ -n "${KEYCHAIN_PATH:-}" ] && keychain_args=(--keychain "$KEYCHAIN_PATH")
    productbuild --resources "$PACKAGING_DIR/resources" --distribution "$WORK_DIR/distribution.xml" \
        --package-path "$WORK_DIR" --sign "$DEVELOPER_ID_INSTALLER" "${keychain_args[@]}" --timestamp "$ARTIFACT"
else
    echo "No DEVELOPER_ID_INSTALLER set - unsigned installer (fine for local testing, not for distribution)."
    productbuild --resources "$PACKAGING_DIR/resources" --distribution "$WORK_DIR/distribution.xml" \
        --package-path "$WORK_DIR" "$ARTIFACT"
fi

if [ -n "${NOTARIZATION_PASSWORD:-}" ]; then
    echo "Notarizing..."
    xcrun notarytool submit "$ARTIFACT" --apple-id "$NOTARIZATION_USERNAME" --password "$NOTARIZATION_PASSWORD" \
        --team-id "$NOTARIZATION_TEAM_ID" --wait
    xcrun stapler staple "$ARTIFACT"
fi

echo "Wrote $ARTIFACT"
