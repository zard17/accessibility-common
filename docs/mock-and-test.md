# Mock Architecture, Build System & Test Infrastructure

> Parent document: [architecture-overview.md](architecture-overview.md)

---

## Mock Architecture

The `MockDBusWrapper` enables testing the full bridge pipeline in-process without a real D-Bus daemon:

```
Test App
  |
  +-- Install MockDBusWrapper
  +-- Create TestAccessible tree
  +-- bridge->Initialize() + ApplicationResumed()
  |     |
  |     +-- ForceUp registers interfaces in MockDBusWrapper's registry
  |     +-- Canned responses handle init calls (GetAddress, Embed, etc.)
  |
  +-- DBusClient.method("GetRole").call()
        |
        +-- Creates MockMessage with path/interface/member
        +-- MockDBusWrapper::eldbus_proxy_send_and_block_impl()
        +-- RouteMethodCall: looks up (path, interface, member) in registry
        +-- Invokes registered callback (same code path as production)
        +-- Returns MockMessage with serialized response
        +-- DBusClient deserializes response using same dbus.h templates
```

### Canned Responses

The mock pre-populates responses for external AT-SPI services called during bridge initialization:

| Service | Method | Response |
|---|---|---|
| org.a11y.Bus | GetAddress | `"unix:path=/tmp/mock-atspi"` |
| org.a11y.atspi.Registry | GetRegisteredEvents | Empty event list |
| org.a11y.atspi.Socket | Embed | Dummy parent Address |
| org.a11y.atspi.Socket | Unembed | Success (no-op) |
| org.a11y.Status | IsEnabled | `true` |
| org.a11y.Status | ScreenReaderEnabled | `true` |

---

## Build System

The CMake build supports three configurations controlled by `ENABLE_ACCESSIBILITY` and `eldbus_available`:

```
ENABLE_ACCESSIBILITY=ON + eldbus:    Full bridge + dbus-tizen.cpp (production)
ENABLE_ACCESSIBILITY=ON + no eldbus: Full bridge + dbus-stub.cpp  (test/CI)
ENABLE_ACCESSIBILITY=OFF:            DummyBridge only              (no a11y)
```

### dbus-stub.cpp

Provides the same portable symbols as `dbus-tizen.cpp` (static variables, `DBusClient`/`DBusServer` constructors, `DBusWrapper::Install`/`Installed`) but without the EFL `DefaultDBusWrapper`. Callers must install their own `DBusWrapper` via `Install()` before any D-Bus operations.

### Test Build

`BUILD_TESTS=ON` adds the `accessibility-test` executable. The test compiles all bridge sources directly rather than linking against the shared library, because the library uses `-fvisibility=hidden` which hides internal symbols (`DBusWrapper`, `DBusClient`, etc.) that the test needs to access.

```
cmake .. -DENABLE_ACCESSIBILITY=ON -DBUILD_TESTS=ON -DENABLE_PKG_CONFIGURE=OFF
make
./accessibility-test
```

### Inspector Build

Both inspectors compile bridge sources directly (like tests) and use MockDBusWrapper for in-process IPC.

```
# CLI inspector
cmake .. -DENABLE_ACCESSIBILITY=ON -DBUILD_INSPECTOR=ON -DENABLE_PKG_CONFIGURE=OFF
make && ./accessibility-inspector

# Web inspector
cmake .. -DENABLE_ACCESSIBILITY=ON -DBUILD_WEB_INSPECTOR=ON -DENABLE_PKG_CONFIGURE=OFF
make && ./accessibility-web-inspector
```

---

## Test Coverage (31 tests)

| Category | Tests |
|----------|-------|
| Bridge lifecycle | Initialize, ForceUp, Terminate |
| Accessible interface | GetRole, GetName, GetChildCount, GetChildAtIndex, GetState |
| Component interface | GetExtents |
| Bridge API | FindByPath, AddTopLevelWindow |
| D-Bus serialization | string, uint32, enum, struct, array, bitset, Address round-trips |

---

## Known Limitations

- `Accessible*` deserialization from D-Bus messages requires `CurrentBridgePtr` context (only available during server-side callback processing). Client-side tests verify the serialized `Address` instead.
- The mock's `addIdle`/`createTimer` callbacks execute synchronously and immediately. This is sufficient for testing but does not simulate real async behavior.
