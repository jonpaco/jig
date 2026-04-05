#!/bin/bash
set -euo pipefail

# Build libjig.so for Android (arm64-v8a and x86_64).
#
# Usage:
#   ./scripts/build-so.sh [ndk-path] [output-dir]
#
# Arguments:
#   ndk-path    Path to Android NDK root. Defaults to $ANDROID_NDK_HOME,
#               then $ANDROID_HOME/ndk/<latest>.
#   output-dir  Where to place the per-ABI .so files.
#               Defaults to packages/native/build/android.
#
# Output structure:
#   <output-dir>/arm64-v8a/libjig.so
#   <output-dir>/x86_64/libjig.so

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
JNI_DIR="$REPO_ROOT/packages/native/android/jni"

# --- Resolve NDK ----------------------------------------------------------

NDK="${1:-}"

if [ -z "$NDK" ]; then
    if [ -n "${ANDROID_NDK_HOME:-}" ]; then
        NDK="$ANDROID_NDK_HOME"
    elif [ -n "${ANDROID_HOME:-}" ] && [ -d "$ANDROID_HOME/ndk" ]; then
        # Pick the newest NDK version installed
        NDK="$(ls -1d "$ANDROID_HOME/ndk"/*/ 2>/dev/null | sort -V | tail -1)"
        NDK="${NDK%/}"
    fi
fi

if [ -z "$NDK" ] || [ ! -d "$NDK" ]; then
    echo "ERROR: Android NDK not found." >&2
    echo "Set ANDROID_NDK_HOME or ANDROID_HOME, or pass the path as argument." >&2
    exit 1
fi

TOOLCHAIN="$NDK/build/cmake/android.toolchain.cmake"
if [ ! -f "$TOOLCHAIN" ]; then
    echo "ERROR: NDK toolchain file not found at $TOOLCHAIN" >&2
    exit 1
fi

echo "Using NDK: $NDK"

# --- Resolve output dir ---------------------------------------------------

OUTPUT_DIR="${2:-$REPO_ROOT/packages/native/build/android}"

# --- Build for each ABI ---------------------------------------------------

ABIS=("arm64-v8a" "x86_64")

for ABI in "${ABIS[@]}"; do
    echo ""
    echo "=== Building libjig.so for $ABI ==="

    BUILD_DIR="$REPO_ROOT/packages/native/build/cmake-android-$ABI"
    mkdir -p "$BUILD_DIR"

    cmake -S "$JNI_DIR" -B "$BUILD_DIR" \
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
        -DANDROID_ABI="$ABI" \
        -DANDROID_PLATFORM=android-26 \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        -DCMAKE_BUILD_TYPE=Release

    cmake --build "$BUILD_DIR" --parallel

    # Copy .so to output
    mkdir -p "$OUTPUT_DIR/$ABI"
    cp "$BUILD_DIR/libjig.so" "$OUTPUT_DIR/$ABI/libjig.so"

    echo "Built: $OUTPUT_DIR/$ABI/libjig.so"
done

echo ""
echo "=== Done ==="
echo "Output: $OUTPUT_DIR"
ls -lh "$OUTPUT_DIR"/*/libjig.so
