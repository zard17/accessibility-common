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

### Phase 3: Mock D-Bus + Test Infrastructure (Current)

Created an in-process mock D-Bus layer and test framework to exercise the full bridge serialization pipeline without real D-Bus IPC.

**Deliverables:**
- `test/mock/mock-dbus-wrapper.h/.cpp` - MockDBusWrapper implementing all DBusWrapper virtual methods in-memory
- `test/test-accessible.h/.cpp` - Concrete Accessible+Component for building test trees
- `test/test-app.cpp` - Test application exercising bridge initialization, property access, method calls
- CMake `BUILD_TESTS` option

**Test coverage:**
- Bridge lifecycle: Initialize, ForceUp, Terminate
- Accessible interface: GetRole, GetName, GetChildCount, GetChildAtIndex, GetState
- Component interface: GetExtents
- Bridge API: FindByPath, AddTopLevelWindow
- D-Bus serialization round-trips: string, uint32, enum, struct, array, bitset

### Phase 4: Alternative IPC Backend (Planned)

Abstract the IPC layer to support TIDL (Tizen Interface Definition Language) as an alternative to D-Bus, similar to Android's AIDL approach.

**Goals:**
- Define an IPC-agnostic interface layer between the bridge and transport
- Implement TIDL backend for Tizen
- Keep D-Bus backend for backward compatibility
- Use MockDBusWrapper pattern as reference for the abstraction boundary

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
