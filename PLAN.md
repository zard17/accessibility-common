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

### Phase 4: IPC Abstraction Layer (Complete — Phase 4a)

Introduced a protocol-neutral IPC abstraction layer so the bridge logic can work with multiple IPC backends. D-Bus is wrapped as the first backend; all 31 existing tests pass through the new layer.

**Deliverables (Phase 4a):**
- `ipc/ipc-result.h` - Protocol-neutral `Ipc::ValueOrError<T>`, `Ipc::Error`, `Ipc::ErrorType`; `DBus::ValueOrError` is now an alias
- `ipc/ipc-server.h` - Abstract `Ipc::Server` interface (`addInterface`, `getBusName`, `getCurrentObjectPath`)
- `ipc/ipc-client.h` - Abstract `Ipc::Client` interface (`isConnected`, `operator bool`)
- `ipc/ipc-interface-description.h` - Base `Ipc::InterfaceDescription`; `DBus::DBusInterfaceDescription` inherits from it
- `dbus/dbus-ipc-server.h` - `Ipc::DbusIpcServer` wrapping `DBus::DBusServer`
- `dbus/dbus-ipc-client.h` - `Ipc::DbusIpcClient` wrapping `DBus::DBusClient`
- `BridgeBase` refactored: `mDbusServer` / `mConnectionPtr` replaced with `std::unique_ptr<Ipc::Server> mIpcServer`
- All bridge-*.cpp modules use `mIpcServer->addInterface()` instead of `mDbusServer.addInterface()`
- Signal emission uses `getDbusServer().emit2<>()` accessor (D-Bus-specific, to be abstracted in Phase 4b)
- `BridgeImpl` uses `getConnection()` accessor instead of `mConnectionPtr`
- Backward-compatible: `DBus::ValueOrError`, `DBus::Error`, `DBus::ErrorType` are aliases to `Ipc::` types

### Phase 4b: TIDL Backend (Planned)

Implement a TIDL backend using the IPC abstraction layer from Phase 4a.

**Goals:**
- Implement `Ipc::Server` and `Ipc::Client` for TIDL (generated proxy/stub from `.tidl` definitions)
- Abstract signal emission (`emit2` → virtual `emitSignal`) so TIDL can use delegate callbacks
- Abstract D-Bus-specific serialization types (`EldbusVariant`, `ObjectPath`) at the interface boundary
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
