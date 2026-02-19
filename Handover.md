# Handover: TIDL IPC Backend (Phase 2.6 Stage A)

## What was done

### TIDL Backend Scaffold (Stage A)

Added a pluggable TIDL backend alongside the existing D-Bus backend so the accessibility bridge can communicate via Tizen's native `rpc_port` direct P2P IPC instead of a D-Bus daemon.

#### New files created (`accessibility/internal/bridge/tidl/`)

| File | Description |
|------|-------------|
| `accessibility-service.tidl` | TIDL interface definition for all 12 bridge modules + status/registry/key events/TTS |
| `tidl-interface-description.h` | `TidlInterfaceDescription` with type-erased `addMethod<T>`, `addProperty<T>`, `addSignal<ARGS...>` matching DBusInterfaceDescription API |
| `tidl-ipc-server.h/cpp` | `TidlIpcServer : Ipc::Server` — stores interface descriptions, tracks dispatch state (scaffold: logs, no real IPC) |
| `tidl-transport-factory.h` | `TidlTransportFactory : Ipc::TransportFactory` — creates all TIDL IPC components |
| `tidl-status-monitor.h` | Scaffold: returns `isEnabled=true` |
| `tidl-key-event-forwarder.h` | Scaffold: returns `consumed=false` |
| `tidl-direct-reading-client.h` | Scaffold: returns errors |
| `tidl-registry-client.h` | Scaffold: returns empty event list |
| `tidl-socket-client.h` | Scaffold: returns errors |

#### Modified files

| File | Change |
|------|--------|
| `bridge/file.list` | Added `accessibility_common_tidl_src_files` |
| `build/tizen/CMakeLists.txt` | Added `ENABLE_TIDL` option, `rpc-port` pkg-config, TIDL backend selection, test binary support |
| `bridge/bridge-impl.cpp` | `#ifdef ENABLE_TIDL_BACKEND` selects `TidlTransportFactory` vs `DbusTransportFactory` |
| `bridge/bridge-base.h` | Updated `AddFunctionToInterface`, `AddGetPropertyToInterface`, `AddSetPropertyToInterface`, `AddGetSetPropertyToInterface` to cast to `TidlInterfaceDescription` when TIDL is active |
| `docs/architecture-overview.md` | Updated Phase 2.6 status |

#### Key design decisions

- **Object path problem**: Every TIDL method takes `objectPath` parameter to simulate D-Bus object dispatch
- **InterfaceDescription**: `TidlInterfaceDescription` uses `std::any` for type-erased method/property storage; same template API as `DBusInterfaceDescription`
- **Compile-time backend selection**: `#ifdef ENABLE_TIDL_BACKEND` in bridge-base.h helpers avoids runtime overhead
- **D-Bus stub still compiled**: TIDL build includes `dbus-stub.cpp` because bridge modules still reference `DBus::` types for `ValueOrError` aliases

### Test results

- D-Bus backend (ENABLE_TIDL=OFF): **56/56 passed** — no regressions
- TIDL shared library: compiles and links
- TIDL test binary: compiles; bridge lifecycle works (all 14 interfaces registered). Method-level tests fail as expected (MockDBusWrapper is D-Bus-specific)

## What's next

### Phase 2.6 Stages B & C (require Tizen SDK)
1. Run `tidlc` to generate C++ stub/proxy from `accessibility-service.tidl`
2. Implement `TidlIpcServer` dispatch using generated stub
3. Implement all 5 client wrappers using generated proxies
4. Test with real screen reader on Tizen

### Phase 3: AccessibilityService base class
- `NodeProxy` interface (~40 methods: getName, getRole, getStates, getNeighbor, ...)
- `AppRegistry` (desktop, active window, app lifecycle)
- `GestureProvider` (gesture → navigation mapping)
- `AccessibilityService` base class with virtual hooks

## Verification commands

```bash
cd ~/tizen/accessibility-common/build/tizen/build

# Default D-Bus backend (all tests pass)
cmake .. -DENABLE_ATSPI=ON -DBUILD_TESTS=ON -DENABLE_PKG_CONFIGURE=OFF
make -j8 && ./accessibility-test

# TIDL backend (compiles, bridge lifecycle works)
cmake .. -DENABLE_ATSPI=ON -DENABLE_TIDL=ON -DENABLE_PKG_CONFIGURE=OFF
make -j8
```
