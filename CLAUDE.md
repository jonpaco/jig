# Jig (Code)

## Overview

This is the code monorepo for Jig. Planning, specs, and research live in `../jig-planning/`.

## Repository Structure

pnpm monorepo:

| Package | Path | Purpose |
|---------|------|---------|
| `@jig/protocol` | `packages/protocol/` | JSON Schema spec + generated TypeScript types |
| `@jig/native` | `packages/native/` | Native source (build-only) — server, touch injection, view walking |
| `@jig/sdk` | `packages/sdk/` | TypeScript client — commands, events, element selection |
| `@jig/cli` | `packages/cli/` | CLI — `jig launch`, `jig status`, `jig wait`, `jig report` |
| `@jig/device` | `packages/device/` | Device management — deterministic emulator & simulator lifecycle |

| Other | Path | Purpose |
|-------|------|---------|
| Docs | `docs/` | Device management guide |

### Android Build Tools

| Tool | Path | Build command | Output |
|------|------|---------------|--------|
| libjig.so | `scripts/build-so.sh` | `bash scripts/build-so.sh` | `packages/native/build/android/<abi>/libjig.so` |
| jig-helpers.dex | `packages/native/scripts/build-dex.sh` | `pnpm build:dex` (in `@jig/native`) | `packages/native/build/dex/jig-helpers.dex` |
| jig-dex-patcher.jar | `packages/native/android/dex-patcher/` | `pnpm build:dex-patcher` (in `@jig/native`) | `packages/native/android/dex-patcher/build/libs/jig-dex-patcher.jar` |

- **build-dex.sh**: Compiles `packages/native/android/jni/*.java` → `.class` (javac) → `.dex` (d8). Requires `ANDROID_HOME`.
- **dex-patcher**: Gradle project using dexlib2. Inserts `System.loadLibrary("jig")` into a DEX class's `<clinit>`. Used by the inject pipeline (`packages/cli/src/android/inject.ts`).
- **zip.ts** (`packages/cli/src/android/zip.ts`): ZIP manipulation utilities for APK injection — add/replace/extract entries without apktool. Preserves original compression methods (critical: `resources.arsc` and `.so` must stay uncompressed).

### Android Build Tools

| Tool | Path | Build command | Output |
|------|------|---------------|--------|
| libjig.so | `scripts/build-so.sh` | `bash scripts/build-so.sh` | `packages/native/build/android/<abi>/libjig.so` |
| jig-helpers.dex | `packages/native/scripts/build-dex.sh` | `pnpm build:dex` (in `@jig/native`) | `packages/native/build/dex/jig-helpers.dex` |
| jig-dex-patcher.jar | `packages/native/android/dex-patcher/` | `pnpm build:dex-patcher` (in `@jig/native`) | `packages/native/android/dex-patcher/build/libs/jig-dex-patcher.jar` |

- **build-dex.sh**: Compiles `packages/native/android/jni/*.java` → `.class` (javac) → `.dex` (d8). Requires `ANDROID_HOME`.
- **dex-patcher**: Gradle project using dexlib2. Inserts `System.loadLibrary("jig")` into a DEX class's `<clinit>`. Used by the inject pipeline (`packages/cli/src/android/inject.ts`).
- **zip.ts** (`packages/cli/src/android/zip.ts`): ZIP manipulation utilities for APK injection — add/replace/extract entries without apktool. Preserves original compression methods (critical: `resources.arsc` and `.so` must stay uncompressed).

## Conventions

### Protocol

- Wire format is JSON-RPC 2.0 — commands have `id`, events don't
- All protocol types live in `@jig/protocol` and are generated from JSON Schema
- Selectors are composable JSON objects, not strings or DSLs
- Three-message handshake: `server.hello` → `client.hello` → `session.ready`

### Native (iOS)

- Touch injection: KIF-style in-process `UITouch` + `IOHIDEvent` via private APIs
- WebSocket server: `NWListener` (Network.framework)
- View hierarchy: recursive `UIView.subviews` traversal
- Screenshots: `CALayer.render(in:)` default, `drawHierarchy` for accuracy
- Network interception: `NSURLProtocol` subclass
- testID maps to `accessibilityIdentifier`

### Native (Android)

- Touch injection: `View.dispatchTouchEvent(MotionEvent)` on DecorView
- WebSocket server: shared C core (libwebsockets), same as iOS
- View hierarchy: recursive `ViewGroup.getChildAt()` traversal
- Screenshots: `PixelCopy` via `JigScreenshotHelper` JNI helper on main thread (reads from SurfaceFlinger, works on headless emulators)
- Network interception: OkHttp interceptor via `OkHttpClientFactory`
- testID maps to `contentDescription`
- JNI helpers (`JigMainThreadRunner`, `JigActivityCallbacks`, `JigScreenshotHelper`) are compiled from Java source in `packages/native/android/jni/*.java` to `jig-helpers.dex` via `d8`. The inject pipeline adds the DEX as a multidex entry alongside `libjig.so`
- APK injection is apktool-free: direct ZIP manipulation + `jig-dex-patcher.jar` (dexlib2) for surgical `<clinit>` patching. Resources are never decoded or rebuilt

### TypeScript

- Strict TypeScript throughout
- Tests with Jest
- pnpm for package management
- Changesets for versioning

### Device Management

- `jig.devices.yml` at repo root declares named device profiles (Android emulators, iOS simulators)
- `jig-device check <profile>` validates all dependencies with actionable error messages
- `jig-device boot <profile>` creates and boots a device deterministically from the manifest
- `jig-device list` / `jig-device kill` for lifecycle management
- `--verbose` or `JIG_DEBUG=1` for detailed logging of every command and decision
- Android: writes AVD `config.ini` directly (bypasses `avdmanager`), uses on-device readiness script (`jig-ready.sh`)
- iOS: wraps `xcrun simctl`, erases before boot for determinism, disables keyboard autocorrect
- `jig launch` auto-boots a device if none is running; `--device <profile>` for explicit selection, `--no-device` to skip

### Testing

- TDD: write failing test first, then implement
- Test against Habit Tracker (`jonpaco/open-source-habit-tracker-app`) for fast development loop
- Test against Bluesky Social and Artsy Eigen for production validation

## Branching

Use `feature/**` or `fix/**` branches. Never commit directly to `main`.

## Git Commits

Do not append "Co-Authored-By" lines to commit messages.
