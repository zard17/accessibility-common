# accessibility-common: Project Plan

## Overview

Refactor DALi accessibility into a standalone, toolkit-agnostic accessibility library that abstracts the hard AT-SPI/D-Bus dependency behind pluggable interfaces. This enables:

- Cross-platform accessibility support (Tizen, Linux, macOS, future platforms)
- Alternative IPC backends (TIDL/RPC alongside or replacing D-Bus)
- Testability without real D-Bus infrastructure

## Phases

### Phase 1: Extract accessibility-common library (Complete)

Moved all accessibility bridge code from `dali-adaptor` into a standalone `accessibility-common` library with its own build system.

**Deliverables:**
- `accessibility/api/` - Public API (Accessible, Component, Bridge interfaces, enums, types)
- `accessibility/internal/bridge/` - AT-SPI bridge implementation (bridge-base, bridge-impl, bridge-accessible, bridge-component, etc.)
- `accessibility/internal/bridge/dbus/` - D-Bus serialization layer (dbus.h, dbus-tizen.cpp)
- `build/tizen/CMakeLists.txt` - Standalone build with `ENABLE_ATSPI` option

### Phase 2: Make library toolkit-agnostic (Complete)

Removed all DALi/EFL compile-time dependencies from the library's public and internal APIs. Platform integration is done via runtime callbacks.

**Deliverables:**
- `PlatformCallbacks` struct for idle/timer management, toolkit info
- `Accessible` pure virtual interface (no DALi Actor dependency)
- `Bridge` interface with `AddAccessible()` / `GetAccessible()` for external registration
- Platform-agnostic logging via `ACCESSIBILITY_LOG_*` macros
- `dbus-stub.cpp` for building without eldbus (macOS, non-Tizen Linux)

### Phase 3: Mock D-Bus + Test Infrastructure (Complete)

Created an in-process mock D-Bus layer and test framework to exercise the full bridge serialization pipeline without real D-Bus IPC. All 31 tests pass on macOS.

**Deliverables:**
- `test/mock/mock-dbus-wrapper.h/.cpp` - MockDBusWrapper implementing all DBusWrapper virtual methods in-memory
- `test/test-accessible.h/.cpp` - Concrete Accessible+Component for building test trees
- `test/test-app.cpp` - Test application exercising bridge initialization, property access, method calls
- `accessibility/internal/bridge/dbus/dbus-stub.cpp` - D-Bus stub backend for platforms without eldbus
- CMake `BUILD_TESTS` option
- Test executable compiles bridge sources directly (shared library uses `-fvisibility=hidden`)

**Test coverage (31 tests):**
- Bridge lifecycle: Initialize, ForceUp, Terminate
- Accessible interface: GetRole, GetName, GetChildCount, GetChildAtIndex, GetState
- Component interface: GetExtents
- Bridge API: FindByPath, AddTopLevelWindow
- D-Bus serialization round-trips: string, uint32, enum, struct, array, bitset, Address

### Phase 4: IPC Abstraction Layer (Complete — Phases 4a, 4b, 4c)

Introduced a protocol-neutral IPC abstraction layer so the bridge logic can work with multiple IPC backends. D-Bus is wrapped as the first backend; all 31 existing tests pass through the new layer.

**Deliverables (Phase 4a — IPC Server/Client/InterfaceDescription):**
- `ipc/ipc-result.h` - Protocol-neutral `Ipc::ValueOrError<T>`, `Ipc::Error`, `Ipc::ErrorType`; `DBus::ValueOrError` is now an alias
- `ipc/ipc-server.h` - Abstract `Ipc::Server` interface (`addInterface`, `getBusName`, `getCurrentObjectPath`)
- `ipc/ipc-client.h` - Abstract `Ipc::Client` interface (`isConnected`, `operator bool`)
- `ipc/ipc-interface-description.h` - Base `Ipc::InterfaceDescription`; `DBus::DBusInterfaceDescription` inherits from it
- `dbus/dbus-ipc-server.h` - `Ipc::DbusIpcServer` wrapping `DBus::DBusServer`
- `dbus/dbus-ipc-client.h` - `Ipc::DbusIpcClient` wrapping `DBus::DBusClient`
- `BridgeBase` refactored: `mDbusServer` / `mConnectionPtr` replaced with `std::unique_ptr<Ipc::Server> mIpcServer`
- All bridge-*.cpp modules use `mIpcServer->addInterface()` instead of `mDbusServer.addInterface()`
- `BridgeImpl` uses `getConnection()` accessor instead of `mConnectionPtr`
- Backward-compatible: `DBus::ValueOrError`, `DBus::Error`, `DBus::ErrorType` are aliases to `Ipc::` types

**Deliverables (Phase 4b — Signal Emission Abstraction):**
- `ipc/ipc-server.h` - Added `Ipc::SignalVariant` (variant of int, string, Address, Rect<int>) and `emitSignal()` pure virtual
- `dbus/dbus-ipc-server.h` - Implemented `emitSignal()` using `std::visit` to dispatch to D-Bus `emit2<>()`
- `bridge-object.cpp` - Replaced all 11 `getDbusServer().emit2<>()` calls with `mIpcServer->emitSignal()` — now fully D-Bus-free
- `bridge-base.h` - `getDbusServer()` made private; bridge modules no longer have direct D-Bus access

**Deliverables (Phase 4c — Accessibility Inspector):**
- `tools/inspector/inspector.cpp` - Interactive CLI accessibility inspector with tree display, focus navigation, element reading, and TTS
- `tools/inspector/tts-mac.mm` - macOS TTS via AVSpeechSynthesizer
- `tools/inspector/tts-stub.cpp` - Fallback TTS (prints to console)
- `tools/inspector/tts.h` - TTS interface header
- CMake `BUILD_INSPECTOR` option
- Demo tree: Window with Header/Content/Footer panels, buttons, slider, labels
- Navigation uses bridge's `GetNeighbor()` which walks HIGHLIGHTABLE elements

### Phase 4d: Web-Based GUI Accessibility Inspector (Complete)

Browser-based accessibility inspector served via embedded HTTP server.

**Deliverables:**
- `tools/inspector/query-engine.h/.cpp` - Reusable `InspectorEngine::AccessibilityQueryEngine` extracted from inspector.cpp
- `tools/inspector/web-inspector.cpp` - HTTP server with REST API (4 endpoints: tree, element, navigate, HTML)
- `tools/inspector/web-inspector-resources.h` - Embedded HTML/CSS/JS frontend as C++ raw string literals
- `third-party/cpp-httplib/httplib.h` - Vendored cpp-httplib v0.18.3 (MIT, single-header)
- CMake `BUILD_WEB_INSPECTOR` option
- Dark-themed Catppuccin UI with tree panel, detail panel, keyboard shortcuts, Web Speech API TTS
- Demo tree updated: labels are HIGHLIGHTABLE (not FOCUSABLE), branding changed to Tizen

### Phase 4e: macOS NSAccessibility Backend (Planned)

Create a native macOS accessibility backend using NSAccessibilityElement.

**Goals:**
- `DaliAccessibleNode` — NSAccessibilityElement subclass wrapping Accessible objects
- VoiceOver integration via accessibilityChildren, accessibilityHitTest, accessibilityFocusedUIElement
- NSAccessibilityPostNotification for state/focus/value changes
- Requires a rendering surface on macOS (NSWindow + OpenGL/Metal view)

### Phase 4f: TIDL Backend (Planned)

Implement a TIDL backend using the IPC abstraction layer.

**Goals:**
- Implement `Ipc::Server` and `Ipc::Client` for TIDL (generated proxy/stub from `.tidl` definitions)
- TIDL backend maps `emitSignal()` to delegate callbacks
- Runtime backend selection (D-Bus vs TIDL) via configuration or environment variable

### Phase 5: Toolkit Integration (Planned)

Integrate accessibility-common back into DALi as a dependency, replacing the current in-tree accessibility code.

**Goals:**
- `dali-adaptor` depends on `accessibility-common` instead of containing accessibility code
- `dali-toolkit` ControlAccessible implements `Accessible` interface
- Platform callbacks wired through DALi adaptor lifecycle
- Zero behavior change for existing accessibility consumers

## Build Configurations

| Configuration | ENABLE_ATSPI | eldbus | D-Bus Backend | Use Case |
|---|---|---|---|---|
| Full (Tizen) | ON | Available | dbus-tizen.cpp | Production on Tizen |
| Stub (macOS/CI) | ON | Not available | dbus-stub.cpp | Testing with MockDBusWrapper |
| Dummy | OFF | N/A | dummy-atspi.cpp | No accessibility support |
