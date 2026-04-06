#!/bin/bash
set -euo pipefail

# Compile Jig Java helper classes into jig-helpers.dex.
#
# Usage:
#   ./scripts/build-dex.sh [output-dir]
#
# Requires:
#   - ANDROID_HOME set (for android.jar and d8)
#   - javac (JDK)
#
# Output:
#   <output-dir>/jig-helpers.dex

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
NATIVE_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
JAVA_SRC_DIR="$NATIVE_ROOT/android/jni"
OUTPUT_DIR="${1:-$NATIVE_ROOT/build/dex}"

# --- Resolve Android SDK paths ---

if [ -z "${ANDROID_HOME:-}" ]; then
    echo "ERROR: ANDROID_HOME not set." >&2
    exit 1
fi

# Find android.jar (needed for compilation against Android APIs)
PLATFORM_DIR="$(ls -1d "$ANDROID_HOME/platforms/android-"* 2>/dev/null | sort -V | tail -1)"
if [ -z "$PLATFORM_DIR" ] || [ ! -f "$PLATFORM_DIR/android.jar" ]; then
    echo "ERROR: No Android platform found in $ANDROID_HOME/platforms/" >&2
    exit 1
fi
ANDROID_JAR="$PLATFORM_DIR/android.jar"

# Find d8
BUILD_TOOLS_DIR="$(ls -1d "$ANDROID_HOME/build-tools/"* 2>/dev/null | sort -V | tail -1)"
if [ -z "$BUILD_TOOLS_DIR" ]; then
    echo "ERROR: No build-tools found in $ANDROID_HOME/build-tools/" >&2
    exit 1
fi
D8="$BUILD_TOOLS_DIR/d8"

echo "Using platform: $PLATFORM_DIR"
echo "Using build-tools: $BUILD_TOOLS_DIR"

# --- Compile Java → .class files ---

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

JAVA_FILES=(
    "$JAVA_SRC_DIR/JigMainThreadRunner.java"
    "$JAVA_SRC_DIR/JigActivityCallbacks.java"
    "$JAVA_SRC_DIR/JigScreenshotHelper.java"
)

echo "Compiling Java sources..."
javac -source 11 -target 11 \
    -bootclasspath "$ANDROID_JAR" \
    -classpath "$ANDROID_JAR" \
    -d "$TMP_DIR" \
    "${JAVA_FILES[@]}"

# --- Convert .class → DEX ---

mkdir -p "$OUTPUT_DIR"

echo "Converting to DEX..."
"$D8" --min-api 26 --output "$OUTPUT_DIR" "$TMP_DIR"/jig/*.class

# d8 outputs classes.dex — rename to jig-helpers.dex
mv "$OUTPUT_DIR/classes.dex" "$OUTPUT_DIR/jig-helpers.dex"

echo "Built: $OUTPUT_DIR/jig-helpers.dex"
