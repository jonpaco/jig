# Device Management

`@jig/device` manages Android emulators and iOS simulators for local development and CI. It uses a manifest file (`jig.devices.yml`) to define device profiles, ensuring the same configuration every time.

## Quick Start

```bash
# Check dependencies
jig-device check android-ci

# Boot a device
jig-device boot android-ci

# List running devices
jig-device list

# Kill a device
jig-device kill android-ci

# Kill all
jig-device kill --all
```

## Manifest

`jig.devices.yml` at the repo root defines named profiles:

```yaml
devices:
  android-ci:
    platform: android
    api: 34
    arch: auto          # detects host CPU (arm64-v8a on Apple Silicon, x86_64 on Intel/Linux)
    image: google_apis
    screen: 1080x2400
    ram: 4096
    headless: true

  ios-ci:
    platform: ios
    runtime: latest     # picks highest installed iOS runtime
    device: "iPhone 16"
    headless: true

  android-dev:
    platform: android
    api: 34
    arch: auto
    image: google_apis_playstore
    ram: 4096
    headless: false     # opens emulator window

  ios-dev:
    platform: ios
    runtime: latest
    device: "iPhone 16"
    headless: false     # opens Simulator.app
```

Every field has defaults. A minimal profile needs only `platform`:

```yaml
devices:
  quick:
    platform: android
```

**Android defaults:** `api: 34`, `arch: auto`, `image: google_apis`, `screen: 1080x2400`, `ram: 4096`, `headless: true`

**iOS defaults:** `runtime: latest`, `device: "iPhone 16"`, `headless: true`

If no manifest exists and `jig launch` auto-boots a device, it writes `jig.devices.yml` with the defaults used.

## Commands

### `jig-device check [profile]`

Validates all dependencies for a profile. Checks everything upfront and reports all issues at once with install instructions.

```
$ jig-device check android-ci
Checking android-ci:
  ✓ ANDROID_HOME: /Users/jon/Library/Android/sdk
  ✓ emulator: Android emulator version 36.4.10.0
  ✓ adb: Android Debug Bridge version 1.0.41
  ✓ system image: system-images;android-34;google_apis;arm64-v8a
  ✓ hardware acceleration: HVF
  All checks passed.
```

```
$ jig-device check ios-ci
Checking ios-ci:
  ✓ Xcode: /Applications/Xcode.app/Contents/Developer
  ✓ simctl: available
  ✓ runtime: iOS 18.5 (latest)
  ✓ device type: iPhone 16
  All checks passed.
```

Without a profile name, checks all profiles in the manifest.

### `jig-device boot <profile>`

Boots a device from the manifest. Creates it if it doesn't exist.

```bash
jig-device boot android-dev          # boots with GUI
jig-device boot android-ci           # boots headless
jig-device boot ios-dev              # boots + opens Simulator.app
jig-device boot android-ci -t 180000 # custom timeout (ms)
```

### `jig-device list`

Shows running jig-managed devices with uptime.

```
$ jig-device list
  android-dev (android) — emulator-5554 — 42s
  ios-dev (ios) — A0E9C5EB-6B7F-4DE5-843D-97E1C8887FDE — 15s
```

### `jig-device kill [profile]`

```bash
jig-device kill android-dev   # kill specific device
jig-device kill --all         # kill all jig-managed devices
```

## Verbose Logging

Enable with `--verbose` flag or `JIG_DEBUG=1` environment variable:

```bash
JIG_DEBUG=1 jig-device boot android-ci
```

Verbose output shows every command executed, stdout/stderr, timing, and decision points:

```
  [debug] Loading manifest from ./jig.devices.yml
  [debug] Profile "android-ci": api=34, abi=arm64-v8a, image=google_apis, ram=4096
  [debug] exec: adb devices
  [debug]   exit: 0 (11ms)
  [debug] Spawning: emulator -avd jig-android-ci -no-audio -no-boot-anim -gpu auto
  [debug] Readiness: WAIT boot_completed=
  [debug] Readiness: READY
  Booted in 8s
```

## Integration with `jig launch`

`jig launch` auto-boots a device if none is running:

```bash
jig launch MyApp.apk                      # auto-boots Android if needed
jig launch MyApp.app                      # auto-boots iOS if needed
jig launch MyApp.apk --device android-ci  # boot specific profile
jig launch MyApp.apk --no-device          # skip auto-boot
```

## CI Usage

```yaml
- name: Check device dependencies
  run: npx jig-device check android-ci

- name: Boot emulator
  run: npx jig-device boot android-ci --timeout 120000

- name: Run tests
  run: |
    jig launch "$APK_PATH" --skip-verify
    jig wait --timeout 30000
    jig status

- name: Cleanup
  if: always()
  run: npx jig-device kill --all
```

### Linux CI prerequisites

Android emulators on Linux need KVM and libpulse0:

```yaml
- run: |
    echo 'KERNEL=="kvm", GROUP="kvm", MODE="0666", OPTIONS+="static_node=kvm"' | sudo tee /etc/udev/rules.d/99-kvm4all.rules
    sudo udevadm control --reload-rules
    sudo udevadm trigger --name-match=kvm
    sudo apt-get update -qq && sudo apt-get install -y -qq libpulse0
```

### macOS CI notes

Pin runner versions (e.g., `macos-15`) rather than using `macos-latest`. Use `runtime: latest` in profiles so they adapt to whatever Xcode the runner provides. `jig-device check` catches runtime mismatches.

## How It Works

### Android

- Bypasses `avdmanager` (creates broken configs). Writes `config.ini` directly modeled on Android Studio output.
- Platform-specific emulator flags: Linux uses `-no-window` + `swiftshader_indirect`; macOS skips `-no-window` (headless binary lacks HVF entitlement).
- On-device readiness script (`jig-ready.sh`): pushed to `/data/local/tmp/`, checks `sys.boot_completed`, package manager, and settings provider in a single `adb shell` round-trip.
- Snapshots enabled by default. First boot is slow (~8s cold); subsequent boots load the snapshot (<1s).

### iOS

- Wraps `xcrun simctl`. Erases simulator before boot for determinism.
- Disables keyboard autocorrect and predictive text.
- `simctl bootstatus -b` blocks until fully ready (no polling needed).
- `headless: false` opens Simulator.app; `headless: true` boots without UI.
