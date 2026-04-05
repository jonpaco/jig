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
| `@jig/jest` | `packages/jest/` | Jest preset — setup/teardown, matchers |
| `@jig/cli` | `packages/cli/` | CLI — `jig launch`, `jig status`, `jig wait`, `jig report` |
| `@jig/mcp` | `packages/mcp/` | MCP server — exposes Jig as tools for AI agents |

| Other | Path | Purpose |
|-------|------|---------|
| Example app | `examples/basic-app/` | Tier 1 test target — simple Expo app |
| Docs | `docs/` | Protocol spec, getting started, CI guide |

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
- Screenshots: `View.draw(Canvas)` via JNI on main thread
- Network interception: OkHttp interceptor via `OkHttpClientFactory`
- testID maps to `contentDescription`
- JNI helpers (`JigMainThreadRunner`, `JigActivityCallbacks`) are shipped as pre-written smali in `packages/native/android/smali/` — the inject pipeline copies them into APKs alongside `libjig.so`

### TypeScript

- Strict TypeScript throughout
- Tests with Jest
- pnpm for package management
- Changesets for versioning

### Testing

- TDD: write failing test first, then implement
- Test against `examples/basic-app/` for fast development loop
- Test against Bluesky Social and Artsy Eigen for production validation

## Branching

Use `feature/**` or `fix/**` branches. Never commit directly to `main`.

## Git Commits

Do not append "Co-Authored-By" lines to commit messages.
