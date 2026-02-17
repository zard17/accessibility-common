# Claude Code Instructions for accessibility-common

## Project Overview

Standalone, toolkit-agnostic accessibility library extracted from DALi. Provides the AT-SPI bridge, accessible tree interfaces, and D-Bus serialization layer. Designed to support pluggable IPC backends (D-Bus, TIDL) and be consumable by any UI toolkit.

## Repository Structure

- `accessibility/api/` - Public API: `Accessible`, `Component`, `Bridge` interfaces, enums, types
- `accessibility/public-api/` - Top-level convenience header
- `accessibility/internal/bridge/` - AT-SPI bridge implementation (bridge-base, bridge-impl, bridge-accessible, etc.)
- `accessibility/internal/bridge/ipc/` - IPC abstraction layer: `Ipc::Server`, `Ipc::Client`, `Ipc::InterfaceDescription`, `Ipc::ValueOrError`
- `accessibility/internal/bridge/dbus/` - D-Bus backend: `dbus.h` (serialization templates), `dbus-ipc-server.h` / `dbus-ipc-client.h` (IPC backend wrappers), `dbus-tizen.cpp` (EFL backend), `dbus-stub.cpp` (stub backend)
- `test/` - Mock D-Bus wrapper, TestAccessible, test application
- `tools/inspector/` - Interactive CLI accessibility inspector with TTS
- `build/tizen/` - CMake build system

## Code Style

- 2-space indentation, braces on new lines for namespaces/classes/functions
- `// EXTERNAL INCLUDES` / `// INTERNAL INCLUDES` comment blocks for include grouping
- SCREAMING_SNAKE_CASE for enums, PascalCase for classes, camelCase for methods/variables
- Apache 2.0 license header (Samsung Electronics) on all source files
- Doxygen `@brief` style comments for public API

## Building

```bash
cd build/tizen && mkdir -p build && cd build

# Full build (Tizen with eldbus)
cmake .. -DENABLE_ATSPI=ON

# Stub build (macOS/CI without eldbus)
cmake .. -DENABLE_ATSPI=ON -DENABLE_PKG_CONFIGURE=OFF

# With tests
cmake .. -DENABLE_ATSPI=ON -DBUILD_TESTS=ON -DENABLE_PKG_CONFIGURE=OFF

make -j$(nproc)
```

## Running Tests

```bash
./accessibility-test
# Expected: "=== Results: 31 passed, 0 failed ==="
```

## Running the Accessibility Inspector

The inspector is an interactive CLI tool that demonstrates accessibility working end-to-end. It creates a demo Tizen-like accessible tree, initializes the bridge with MockDBusWrapper, and lets you browse the tree and hear TTS output.

```bash
cd build/tizen && mkdir -p build && cd build

# Build with inspector
cmake .. -DENABLE_ATSPI=ON -DBUILD_INSPECTOR=ON -DENABLE_PKG_CONFIGURE=OFF
make -j$(nproc)

# Run
./accessibility-inspector
```

### Inspector Commands

| Key | Action |
|-----|--------|
| `p` | Print the full accessibility tree (focused element marked with `>>`) |
| `n` | Navigate to **next** focusable element (forward) |
| `b` | Navigate to **previous** focusable element (backward) |
| `c` | Navigate to **first child** of current element |
| `u` | Navigate to **parent** of current element |
| `r` | **Read** current element details (name, role, states, bounds) |
| `s` | **Speak** current element via system TTS |
| `h` | Show help |
| `q` | Quit |

### Demo Tree

```
[WINDOW] "Main Window"
  [PANEL] "Header"
    [PUSH_BUTTON] "Menu"        <- focusable + highlightable
    [LABEL] "My Tizen App"      <- highlightable
  [PANEL] "Content"
    [PUSH_BUTTON] "Play"        <- focusable + highlightable
    [SLIDER] "Volume"           <- focusable + highlightable
    [LABEL] "Now Playing: Bohemian Rhapsody"  <- highlightable
  [PANEL] "Footer"
    [PUSH_BUTTON] "Previous"    <- focusable + highlightable
    [PUSH_BUTTON] "Next"        <- focusable + highlightable
```

Forward navigation (`n`) walks: Menu -> My Tizen App -> Play -> Volume -> Now Playing -> Previous -> Next.

### TTS

- **macOS**: Uses `AVSpeechSynthesizer` — speaks element role and name aloud
- **Other platforms**: Prints `[TTS] ROLE. Name` to console

## Running the Web-Based Accessibility Inspector

The web inspector provides a browser-based GUI for exploring the accessibility tree. It uses the same demo tree and bridge infrastructure as the CLI inspector, served via an embedded HTTP server (cpp-httplib).

```bash
cd build/tizen && mkdir -p build && cd build

# Build with web inspector
cmake .. -DENABLE_ATSPI=ON -DBUILD_WEB_INSPECTOR=ON -DENABLE_PKG_CONFIGURE=OFF
make -j$(nproc)

# Run (default port 8080, or specify custom port)
./accessibility-web-inspector
./accessibility-web-inspector 9000
```

Open `http://localhost:8080` in a browser. The interface provides:
- **Tree panel** (left): visual tree with click-to-select and collapsible nodes
- **Detail panel** (right): element info (name, role, states, bounds, tree position)
- **Navigation buttons**: Prev, Next, Child, Parent, Refresh
- **Keyboard shortcuts**: Tab/Shift+Tab (next/prev), Enter (child), Backspace (parent), S (speak), R (refresh)
- **TTS**: Uses browser's Web Speech API (no server dependency)

### REST API

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | HTML/CSS/JS frontend |
| `/api/tree` | GET | Full tree JSON + current focusedId |
| `/api/element/:id` | GET | Element details |
| `/api/navigate` | POST | Navigate: `{"direction": "next\|prev\|child\|parent"}` |

## Key Design Patterns

- **IPC abstraction layer**: Bridge modules use `Ipc::Server` / `Ipc::Client` / `Ipc::InterfaceDescription` interfaces. D-Bus is the first backend (`Ipc::DbusIpcServer`, `Ipc::DbusIpcClient`). `Ipc::ValueOrError<T>` is protocol-neutral; `DBus::ValueOrError<T>` is a backward-compat alias. Signal emission uses `mIpcServer->emitSignal()` with `Ipc::SignalVariant` payloads.
- **DBusWrapper virtual interface**: All D-Bus operations go through `DBusWrapper::Installed()`. Swap backends via `DBusWrapper::Install(unique_ptr)`.
- **Fallback interface registration**: Bridge modules register at path `"/"` with `fallback=true` via `mIpcServer->addInterface()`. `FindCurrentObject()` resolves the target from the request path.
- **PlatformCallbacks**: Runtime callbacks decouple the bridge from any event loop or toolkit.
- **Feature system**: `Accessible::AddFeature<T>()` / `GetFeature<T>()` for optional interfaces (Action, EditableText, Value, etc.).

## Important Files

- `ipc/ipc-result.h` - Protocol-neutral `Ipc::ValueOrError<T>`, `Ipc::Error`, `Ipc::ErrorType`.
- `ipc/ipc-server.h` - Abstract `Ipc::Server` interface for server-side IPC.
- `ipc/ipc-client.h` - Abstract `Ipc::Client` interface for client-side IPC.
- `ipc/ipc-interface-description.h` - Base class for method/property/signal registration.
- `dbus/dbus-ipc-server.h` - `Ipc::DbusIpcServer` wrapping `DBus::DBusServer`.
- `dbus/dbus-ipc-client.h` - `Ipc::DbusIpcClient` wrapping `DBus::DBusClient`.
- `dbus/dbus.h` - Core D-Bus abstraction (~2700 lines). Contains `DBusWrapper`, `DBusClient`, `DBusServer`, all serialization templates.
- `bridge-impl.cpp` - Bridge lifecycle: `Initialize()`, `ForceUp()`, `ForceDown()`, `SwitchBridge()`.
- `bridge-base.cpp` - `FindCurrentObject()`, `ApplicationAccessible`, interface registration helpers.
- `accessibility-common.h` - D-Bus signature specializations for `Address`, `Accessible*`, `States`.
- `tools/inspector/query-engine.h` - Reusable `InspectorEngine::AccessibilityQueryEngine` class for querying the accessible tree.
- `tools/inspector/web-inspector.cpp` - Web-based inspector HTTP server with REST API.
- `third-party/cpp-httplib/httplib.h` - Vendored cpp-httplib v0.18.3 (MIT, single-header HTTP server).

## Rules

- Do not commit partial or untested work
- When adding new bridge interfaces, follow the `RegisterInterfaces()` pattern in existing bridge-*.cpp files
- When adding new canned responses to MockDBusWrapper, add them in `SetupCannedResponses()`
- Test executable compiles bridge sources directly (not via shared lib) due to `-fvisibility=hidden`

## Documentation
- session 시작 시 PLAN.md 읽어서 흐름 파악
- session 종료 시 다음 session에 넘길 내용 Handover.md에 정리, 기존 Handover.md는 지운다