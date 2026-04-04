#!/bin/bash
set -euo pipefail

# Build Jig.framework for iOS simulator (arm64 + x86_64)
# Usage: ./build-framework.sh [output-dir]
#   output-dir defaults to packages/native/build

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
IOS_DIR="$(cd "$SCRIPT_DIR/../ios" && pwd)"
OUTPUT_DIR="${1:-$SCRIPT_DIR/../build}"

SDK_PATH="$(xcrun --sdk iphonesimulator --show-sdk-path)"
MIN_IOS="15.1"

# Collect all .m source files (exclude Swift module wrapper)
SOURCES=()
while IFS= read -r -d '' file; do
    SOURCES+=("$file")
done < <(find "$IOS_DIR" -name '*.m' -print0)

if [ ${#SOURCES[@]} -eq 0 ]; then
    echo "Error: No .m files found in $IOS_DIR"
    exit 1
fi

echo "Building Jig.framework from ${#SOURCES[@]} source files..."

# Compile for each architecture
for ARCH in arm64 x86_64; do
    ARCH_DIR="$OUTPUT_DIR/obj/$ARCH"
    mkdir -p "$ARCH_DIR"

    echo "  Compiling for $ARCH..."
    for SRC in "${SOURCES[@]}"; do
        OBJ_NAME="$(basename "$SRC" .m).o"
        xcrun clang \
            -fobjc-arc \
            -fmodules \
            -target "${ARCH}-apple-ios${MIN_IOS}-simulator" \
            -isysroot "$SDK_PATH" \
            -I "$IOS_DIR" \
            -c "$SRC" \
            -o "$ARCH_DIR/$OBJ_NAME"
    done

    echo "  Linking $ARCH..."
    xcrun clang \
        -dynamiclib \
        -target "${ARCH}-apple-ios${MIN_IOS}-simulator" \
        -isysroot "$SDK_PATH" \
        -install_name @rpath/Jig.framework/Jig \
        -framework Foundation \
        -framework UIKit \
        -framework Network \
        "$ARCH_DIR"/*.o \
        -o "$ARCH_DIR/Jig"
done

# Create universal binary
echo "  Creating universal binary..."
FRAMEWORK_DIR="$OUTPUT_DIR/Jig.framework"
rm -rf "$FRAMEWORK_DIR"
mkdir -p "$FRAMEWORK_DIR"

xcrun lipo -create \
    "$OUTPUT_DIR/obj/arm64/Jig" \
    "$OUTPUT_DIR/obj/x86_64/Jig" \
    -output "$FRAMEWORK_DIR/Jig"

# Create Info.plist
cat > "$FRAMEWORK_DIR/Info.plist" << 'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>Jig</string>
    <key>CFBundleIdentifier</key>
    <string>com.jigtesting.Jig</string>
    <key>CFBundleName</key>
    <string>Jig</string>
    <key>CFBundlePackageType</key>
    <string>FMWK</string>
    <key>CFBundleVersion</key>
    <string>0.1.0</string>
    <key>CFBundleShortVersionString</key>
    <string>0.1.0</string>
    <key>MinimumOSVersion</key>
    <string>15.1</string>
</dict>
</plist>
PLIST

# Ad-hoc codesign (required on Apple Silicon)
codesign --sign - "$FRAMEWORK_DIR"

# Generate SHA-256 manifest
HASH=$(shasum -a 256 "$FRAMEWORK_DIR/Jig" | awk '{print $1}')
echo "$HASH  Jig.framework/Jig" > "$OUTPUT_DIR/jig-binaries.sha256"

echo "Done: $FRAMEWORK_DIR"
echo "SHA-256: $HASH"

# Clean up intermediate object files
rm -rf "$OUTPUT_DIR/obj"
