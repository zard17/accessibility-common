# Handover: Phase 2 Complete — Bidirectional IPC Abstraction

## What was done

### Phase 2: Bidirectional IPC Abstraction (COMPLETE)

Replaced all direct `DBus::DBusClient` usage in the bridge with abstract IPC interfaces, making the IPC layer pluggable.

#### Phase 2a: Domain-specific IPC client interfaces (5 new headers)
- `ipc/ipc-status-monitor.h` — `Ipc::AccessibilityStatusMonitor`
- `ipc/ipc-key-event-forwarder.h` — `Ipc::KeyEventForwarder`
- `ipc/ipc-direct-reading-client.h` — `Ipc::DirectReadingClient`
- `ipc/ipc-registry-client.h` — `Ipc::RegistryClient`
- `ipc/ipc-socket-client.h` — `Ipc::SocketClient`

#### Phase 2b: Transport Factory (1 new header)
- `ipc/ipc-transport-factory.h` — `Ipc::TransportFactory` abstract factory

#### Phase 2c: createInterfaceDescription
- `ipc/ipc-server.h` — added `virtual unique_ptr<InterfaceDescription> createInterfaceDescription(string)`
- `dbus/dbus-ipc-server.h` — implemented using `DBus::DBusInterfaceDescription`

#### Phase 2d: D-Bus backend implementations (6 new headers)
- `dbus/dbus-status-monitor.h` — `DbusStatusMonitor`
- `dbus/dbus-key-event-forwarder.h` — `DbusKeyEventForwarder`
- `dbus/dbus-direct-reading-client.h` — `DbusDirectReadingClient`
- `dbus/dbus-registry-client.h` — `DbusRegistryClient`
- `dbus/dbus-socket-client.h` — `DbusSocketClient`
- `dbus/dbus-transport-factory.h` — `DbusTransportFactory`

#### Phase 2e: Bridge migration
- `bridge-base.h` — Changed helper signatures (`AddFunctionToInterface` etc.) to take `Ipc::InterfaceDescription&` instead of `DBus::DBusInterfaceDescription&`. Added `mTransportFactory`, `mRegistryClient`. Removed `getConnection()`, `getDbusServer()`.
- `bridge-base.cpp` — `ForceUp()` uses `mTransportFactory->connect()`. `UpdateRegisteredEvents()` uses `mRegistryClient`.
- `bridge-impl.cpp` — Replaced 4 `DBus::DBusClient` members with abstract interfaces. `BridgeImpl()` constructor creates `DbusTransportFactory`. All client operations use abstract interface methods.
- 12 bridge-*.cpp files — `RegisterInterfaces()` migrated from `DBus::DBusInterfaceDescription desc{...}` to `auto desc = mIpcServer->createInterfaceDescription(...)`.

#### Phase 2f: Verification
- Build: SUCCESS (no errors, only pre-existing warnings)
- Tests: **31 passed, 0 failed**

### Plan updated with user-requested additions
Added to PLAN.md:
- **Phase 2.5**: eldbus → GDBus migration plan
- **Phase 2.6**: TIDL IPC backend plan
- **Phase 2.7**: Tree embedding implementation & test plan
- **Registry daemon decision**: Not needed for Phase 2-4. For TIDL, recommend using Tizen `amd`.

## Key design decisions

1. **Helper static_cast pattern**: `AddFunctionToInterface` etc. take `Ipc::InterfaceDescription&` but internally `static_cast` to `DBus::DBusInterfaceDescription&`. Practical trade-off — avoids leaking D-Bus types to bridge modules while keeping template machinery working.

2. **Transport factory in constructor**: `BridgeImpl()` constructor creates `DbusTransportFactory` (not external injection via `CreateBridge()`), because `mTransportFactory` is protected in `BridgeBase`.

3. **All new files are header-only**: No changes needed to `file.list` or CMakeLists.txt.

## Files changed (summary)

**New files (12):** All in `accessibility/internal/bridge/`
- `ipc/`: ipc-status-monitor.h, ipc-key-event-forwarder.h, ipc-direct-reading-client.h, ipc-registry-client.h, ipc-socket-client.h, ipc-transport-factory.h
- `dbus/`: dbus-status-monitor.h, dbus-key-event-forwarder.h, dbus-direct-reading-client.h, dbus-registry-client.h, dbus-socket-client.h, dbus-transport-factory.h

**Modified files (17):**
- ipc-server.h, dbus-ipc-server.h (createInterfaceDescription)
- bridge-base.h, bridge-base.cpp (TransportFactory, RegistryClient, helper signatures)
- bridge-impl.cpp (abstract clients, factory constructor)
- bridge-accessible.cpp, bridge-action.cpp, bridge-application.cpp, bridge-collection.cpp, bridge-component.cpp, bridge-editable-text.cpp, bridge-hyperlink.cpp, bridge-hypertext.cpp, bridge-selection.cpp, bridge-socket.cpp, bridge-text.cpp, bridge-value.cpp (createInterfaceDescription)

## What's next

Per updated plan priority:
1. **Phase 2.7**: Tree embedding tests (add MockSocketClient, unit tests for Embed/Unembed/SetOffset)
2. **Phase 2.5**: eldbus → GDBus migration (dbus-gdbus.cpp implementation)
3. **Phase 3**: AccessibilityService base class (NodeProxy, AppRegistry, GestureProvider)
4. **Phase 4a**: Screen reader C++ rewrite

## Verification commands

```bash
cd ~/tizen/accessibility-common/build/tizen/build
cmake .. -DENABLE_ATSPI=ON -DBUILD_TESTS=ON -DENABLE_PKG_CONFIGURE=OFF
make -j8 && ./accessibility-test  # 31 passed, 0 failed
```
