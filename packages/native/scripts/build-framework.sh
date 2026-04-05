#!/bin/bash
set -euo pipefail

# Build Jig.framework for iOS simulator (arm64 + x86_64)
# Usage: ./build-framework.sh [output-dir]
#   output-dir defaults to packages/native/build

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
NATIVE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
IOS_DIR="$NATIVE_DIR/ios"
CORE_DIR="$NATIVE_DIR/core"
OUTPUT_DIR="${1:-$NATIVE_DIR/build}"

SDK_PATH="$(xcrun --sdk iphonesimulator --show-sdk-path)"
MIN_IOS="15.1"

# --- Step 1: Build C core via CMake for each architecture ---
echo "Building C core libraries..."
for ARCH in arm64 x86_64; do
    CORE_BUILD_DIR="$OUTPUT_DIR/core-build/$ARCH"
    mkdir -p "$CORE_BUILD_DIR"

    echo "  CMake configure for $ARCH..."
    cmake \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        -DCMAKE_C_COMPILER="$(xcrun --sdk iphonesimulator -f clang)" \
        -DCMAKE_SYSTEM_NAME=iOS \
        -DCMAKE_OSX_SYSROOT="$SDK_PATH" \
        -DCMAKE_OSX_ARCHITECTURES="$ARCH" \
        -DCMAKE_OSX_DEPLOYMENT_TARGET="$MIN_IOS" \
        -DCMAKE_SYSTEM_PROCESSOR="$ARCH" \
        -S "$CORE_DIR" \
        -B "$CORE_BUILD_DIR" \
        2>&1 | grep -v "^--"

    echo "  Building for $ARCH..."
    cmake --build "$CORE_BUILD_DIR" --parallel "$(sysctl -n hw.logicalcpu)" 2>&1 | tail -1
done

# --- Step 2: Collect Obj-C shim sources ---
SOURCES=()
while IFS= read -r -d '' file; do
    # Skip JigStandaloneInit.m — it's only for standalone injection, not the framework build
    # Actually we DO want it in the framework (it's the constructor entry point)
    SOURCES+=("$file")
done < <(find "$IOS_DIR" -name '*.m' -print0)

if [ ${#SOURCES[@]} -eq 0 ]; then
    echo "Error: No .m files found in $IOS_DIR"
    exit 1
fi

echo "Building Jig.framework from ${#SOURCES[@]} Obj-C source files + C core..."

# --- Step 3: Compile Obj-C shims and link with C core for each arch ---
for ARCH in arm64 x86_64; do
    ARCH_DIR="$OUTPUT_DIR/obj/$ARCH"
    CORE_BUILD_DIR="$OUTPUT_DIR/core-build/$ARCH"
    mkdir -p "$ARCH_DIR"

    # Find the libwebsockets include directory (generated config headers)
    LWS_INCLUDE_DIR="$CORE_BUILD_DIR/_deps/libwebsockets-build/include"

    echo "  Compiling Obj-C for $ARCH..."
    for SRC in "${SOURCES[@]}"; do
        OBJ_NAME="$(basename "$SRC" .m).o"
        xcrun clang \
            -fobjc-arc \
            -fmodules \
            -target "${ARCH}-apple-ios${MIN_IOS}-simulator" \
            -isysroot "$SDK_PATH" \
            -I "$IOS_DIR" \
            -I "$CORE_DIR" \
            -I "$LWS_INCLUDE_DIR" \
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
        "$CORE_BUILD_DIR/libjig_core.a" \
        "$CORE_BUILD_DIR/libcjson.a" \
        "$CORE_BUILD_DIR/_deps/libwebsockets-build/lib/libwebsockets.a" \
        -o "$ARCH_DIR/Jig"
done

# --- Step 4: Create universal binary ---
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

# Clean up intermediate files
rm -rf "$OUTPUT_DIR/obj" "$OUTPUT_DIR/core-build"
