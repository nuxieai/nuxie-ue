#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"
SCHEME=NuxieUnrealBridge
mkdir -p .build lib
for PLATFORM in ios simulator; do
  DESTINATION='generic/platform=iOS'
  SDK_NAME=iphoneos
  if [[ "$PLATFORM" == simulator ]]; then DESTINATION='generic/platform=iOS Simulator'; SDK_NAME=iphonesimulator; fi
  ARCHIVE="$ROOT_DIR/.build/$PLATFORM.xcarchive"
  DERIVED="$ROOT_DIR/.build/DerivedData-$PLATFORM"
  xcodebuild archive -quiet -scheme "$SCHEME" -destination "$DESTINATION" \
    -archivePath "$ARCHIVE" -derivedDataPath "$DERIVED" \
    -clonedSourcePackagesDirPath "$ROOT_DIR/.build/DerivedData/SourcePackages" \
    SKIP_INSTALL=NO BUILD_LIBRARY_FOR_DISTRIBUTION=YES CODE_SIGNING_ALLOWED=NO
  FRAMEWORK="$ARCHIVE/Products/usr/local/lib/$SCHEME.framework"
  if [[ ! -d "$FRAMEWORK" ]]; then FRAMEWORK="$ARCHIVE/Products/Library/Frameworks/$SCHEME.framework"; fi
  test -d "$FRAMEWORK"
  BUNDLE="$DERIVED/Build/Intermediates.noindex/ArchiveIntermediates/$SCHEME/IntermediateBuildFilesPath/UninstalledProducts/$SDK_NAME/Nuxie_Nuxie.bundle"
  test -d "$BUNDLE"
  ditto "$BUNDLE" "$FRAMEWORK/Nuxie_Nuxie.bundle"
  mkdir -p "$ROOT_DIR/lib/$PLATFORM"
  ZIP="$ROOT_DIR/lib/$PLATFORM/$SCHEME.embeddedframework.zip"
  # Replace this script's generated archive; retain incremental compiler products.
  rm -f "$ZIP"
  # Unreal extracts beside ZipOutputDirectory and expects the archive basename as its root.
  STAGE="$ROOT_DIR/.build/package-$PLATFORM"
  ARCHIVE_ROOT="$SCHEME.embeddedframework"
  mkdir -p "$STAGE/$ARCHIVE_ROOT"
  ditto "$FRAMEWORK" "$STAGE/$ARCHIVE_ROOT/$SCHEME.framework"
  (cd "$STAGE" && /usr/bin/zip -qry -X "$ZIP" "$ARCHIVE_ROOT")
  echo "Created $ZIP"
done

python3 "$ROOT_DIR/../../scripts/write-native-receipt.py" ios
