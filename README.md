# Jig

A direct connection to your running React Native app.

Jig is an open-source framework for programmatic control of running React Native apps. An embedded native module runs a WebSocket server inside your app's dev build, giving any client — TypeScript tests, CI pipelines, or AI agents — a direct wire into the running app.

```
App Process                          External Client
┌────────────────────────┐           ┌──────────────────────┐
│  @jig/native           │           │  @jig/sdk   → tests  │
│  ├── WebSocket :4042   │◄─────────►│  @jig/cli   → shell  │
│  ├── Touch injector    │  JSON-RPC │  @jig/mcp   → AI     │
│  ├── View walker       │           └──────────────────────┘
│  └── Screenshot        │
└────────────────────────┘
```

## Why Jig

Jig moves instrumentation **inside the app process**. Instead of coordinating between a test runner and your app through an external bridge, Jig's server lives in your app and speaks JSON-RPC 2.0 over WebSocket — giving you native touch injection, React component tree access, and screenshot capture through a typed TypeScript SDK.

## Packages

| Package | Purpose |
|---------|---------|
| `@jig/protocol` | JSON Schema spec + generated TypeScript types for the wire protocol |
| `@jig/native` | React Native module — WebSocket server, touch injection, view walking, screenshots |
| `@jig/sdk` | TypeScript client — connect, send commands, subscribe to events |
| `@jig/jest` | Jest preset — setup/teardown, custom matchers |
| `@jig/cli` | CLI — `jig status`, `jig wait`, `jig report` |
| `@jig/mcp` | MCP server — exposes Jig as tools for AI agents |

## Quick Start

### Install

```bash
# Expo
npx expo install @jig/native

# Bare React Native
npm install @jig/native && npx pod-install
```

### Verify

```bash
npx jig status
```

```
✓ Connected to MyApp (com.example.myapp)
  Platform:  ios simulator
  RN:        0.83.4
  Jig:       0.1.0
  Protocol:  jig/1
```

### Write a Test

```bash
npm install -D @jig/sdk @jig/jest
```

```typescript
import { jig } from '@jig/sdk';

describe('habit tracker', () => {
  beforeAll(() => jig.connect());

  it('creates a new habit', async () => {
    await jig.commands.tap({ testID: 'new-habit-button' });
    await jig.commands.type({ testID: 'habit-name-input' }, 'Morning Run');
    await jig.commands.tap({ testID: 'save-button' });

    const element = await jig.query({ text: 'Morning Run' });
    expect(element).toBeVisible();
  });
});
```

### AI Agent (MCP)

```json
{
  "mcpServers": {
    "jig": {
      "command": "jig-mcp",
      "args": ["--port", "4042"]
    }
  }
}
```

Ask Claude Code to "create a new habit called Morning Run" — it connects, taps, types, and verifies.

## How It Works

Jig embeds a WebSocket server in the app's native layer (Objective-C on iOS, Java on Android). The server survives Metro reloads, JS crashes, and hot refreshes — it's always up as long as the app process is alive.

The wire protocol is JSON-RPC 2.0. Commands have an `id` and expect a response. Events are fire-and-forget notifications. A three-message handshake establishes the session:

```
Server                          Client
  │──── server.hello ──────────►│  server announces capabilities
  │◄─── client.hello ──────────│  client declares version + subscriptions
  │──── session.ready ─────────►│  session confirmed
```

Touch injection is in-process — synthetic UITouch/MotionEvent dispatched directly through the OS gesture system. Sub-millisecond latency, works on simulator and device, no permissions or external tooling required.

## Roadmap

Jig is built in vertical slices. Each slice ships iOS + Android together and delivers a working capability end-to-end.

### Core (in progress)

| Slice | Capability | Status |
|-------|-----------|--------|
| 1 | **Connect** — WebSocket server, handshake, `jig status` | Done |
| 2 | **Screenshot** — capture and return the screen | Next |
| 3 | **Find elements** — query by testID, text, component, role | Planned |
| 4 | **Tap** — native touch injection (tap, double-tap, long-press) | Planned |
| 5 | **Type** — text input, clear, replace | Planned |
| 6 | **Scroll** — scroll and swipe gestures | Planned |
| 7 | **Test framework** — Jest preset, matchers, setup/teardown | Planned |

Slice 7 is the **MVP** — enough to install Jig, write real tests, and run them.

### Validation

| Slice | Capability | Status |
|-------|-----------|--------|
| 8 | **Bluesky Social** — prove it works on a production Expo app | Planned |
| 9 | **Artsy Eigen** — prove it works on bare React Native | Planned |

### Advanced

| Slice | Capability | Status |
|-------|-----------|--------|
| 10 | **Events** — subscribe to screen changes, navigation, lifecycle | Planned |
| 11 | **App control** — reset state, deep links, orientation, permissions | Planned |
| 12 | **Network** — intercept and monitor HTTP traffic | Planned |
| 13 | **MCP server** — AI agent integration via Model Context Protocol | Planned |
| 14 | **CI reporting** — JUnit XML, GitHub Actions summaries, artifacts | Planned |

Slice 14 is **V1 complete**.

### Future (V2+)

- State inspection — read Redux/Zustand stores, navigation state, pending requests
- AI ergonomics — `describeScreen`, `findElement(naturalLanguage)`, `explainFailure`
- Developer tools — test recorder, interactive REPL, visual overlay, VS Code extension

## Design Principles

1. **In-process, not external** — the server lives inside the app. No test runner process, no instrumentation APK.
2. **Protocol-first** — the WebSocket protocol is the product. SDK, CLI, and MCP are typed wrappers over it.
3. **Zero production footprint** — `@jig/native` is a no-op in release builds.
4. **AI-native** — JSON-RPC 2.0 protocol designed for LLMs to produce and consume.
5. **Platform-symmetric** — iOS and Android expose identical capabilities through a single protocol.

## License

MIT
