# accessibility-common: Architecture

## Directory Structure

```
accessibility-common/
  accessibility/
    api/                          # Public API
      accessibility.h             # Enums: Role, State, AtspiInterface, CoordinateType, etc.
      accessibility.cpp           # Enum-to-string conversions
      accessibility-bridge.h      # Bridge abstract interface
      accessible.h                # Accessible pure virtual class (+ Component)
      component.h                 # Component pure virtual interface
      types.h                     # Rect<T>, KeyEvent, Signal<>
      log.h / log.cpp             # Platform-agnostic logging
    public-api/
      accessibility-common.h      # Top-level convenience header
    internal/
      bridge/
        bridge-base.h/.cpp        # BridgeBase: connection management, interface registration
        bridge-impl.h/.cpp        # BridgeImpl: full bridge with ForceUp/ForceDown lifecycle
        bridge-accessible.h/.cpp  # AT-SPI Accessible interface registration
        bridge-component.cpp      # AT-SPI Component interface registration
        bridge-action.cpp         # AT-SPI Action interface
        bridge-collection.cpp     # AT-SPI Collection interface
        bridge-*.cpp              # Other AT-SPI interfaces
        bridge-platform.h/.cpp    # PlatformCallbacks, RepeatingTimer
        accessibility-common.h    # D-Bus constants, signature specializations
        ipc/
          ipc-result.h            # Ipc::ValueOrError<T>, Error, ErrorType (protocol-neutral)
          ipc-server.h            # Ipc::Server abstract interface
          ipc-client.h            # Ipc::Client abstract interface
          ipc-interface-description.h  # Ipc::InterfaceDescription base class
        dbus/
          dbus.h                  # DBusWrapper virtual interface, DBusClient/Server, serialization
          dbus-ipc-server.h       # Ipc::DbusIpcServer wrapping DBus::DBusServer
          dbus-ipc-client.h       # Ipc::DbusIpcClient wrapping DBus::DBusClient
          dbus-tizen.cpp          # EFL/eldbus implementation (Tizen only)
          dbus-stub.cpp           # Stub implementation (no EFL required)
          dbus-locators.h         # D-Bus bus/path/interface constants
  test/
    mock/
      mock-dbus-wrapper.h/.cpp    # In-memory mock DBusWrapper
    test-accessible.h/.cpp        # Concrete Accessible for tests
    test-app.cpp                  # Test application
  build/tizen/
    CMakeLists.txt                # Build system
```

## Key Abstractions

### Accessible (api/accessible.h)

Pure virtual interface representing an accessibility node. Any UI toolkit implements this to expose its widget tree to assistive technologies.

```
Accessible
  +-- GetName() / GetDescription() / GetValue()
  +-- GetRole() / GetStates() / GetAttributes()
  +-- GetParent() / GetChildren() / GetChildAtIndex()
  +-- GetAddress()           # Unique ID for IPC routing
  +-- Component interface    # Spatial: GetExtents(), GetLayer(), GrabFocus()
  +-- Feature system         # AddFeature<Action>(), GetFeature<EditableText>()
```

### Bridge (api/accessibility-bridge.h)

Manages the connection between the accessibility tree and assistive technology consumers. Handles registration, lifecycle, and IPC.

```
Bridge
  +-- GetCurrentBridge()     # Singleton access
  +-- Initialize() / Terminate()
  +-- ApplicationResumed() / ApplicationPaused()
  +-- AddAccessible(id, ptr) / GetAccessible(id)
  +-- AddTopLevelWindow(ptr)
  +-- FindByPath(path)       # Resolve IPC address to Accessible*
  +-- IsUp() / ForceUp() / ForceDown()
```

### DBusWrapper (internal/bridge/dbus/dbus.h)

Virtual interface abstracting all D-Bus operations. Allows plugging in different backends (EFL eldbus, mock, future TIDL).

```
DBusWrapper
  +-- Install(unique_ptr<DBusWrapper>)    # Set active backend
  +-- Installed()                          # Get active backend
  +-- Connection / Object / Proxy         # Handle types
  +-- MessageIter                          # Serialization cursor
  +-- add_interface_impl()                 # Server-side interface registration
  +-- eldbus_proxy_send_and_block_impl()   # Client-side method calls
  +-- eldbus_message_iter_*                # Type-safe serialization (12 types)
```

### IPC Abstraction Layer (internal/bridge/ipc/)

Protocol-neutral interfaces that decouple the bridge from any specific IPC backend. Bridge modules interact with `Ipc::Server` and `Ipc::Client` instead of D-Bus types directly.

```
Ipc::Server (abstract)              Ipc::Client (abstract)
  +-- addInterface()                  +-- isConnected()
  +-- getBusName()                    +-- operator bool()
  +-- getCurrentObjectPath()
       |                                   |
       v                                   v
Ipc::DbusIpcServer                  Ipc::DbusIpcClient
  wraps DBus::DBusServer              wraps DBus::DBusClient
  +-- getDbusServer()                 +-- getDbusClient()
  +-- getConnection()

Ipc::InterfaceDescription (base)
  +-- getInterfaceName()
       |
       v
DBus::DBusInterfaceDescription
  +-- addMethod<T>()
  +-- addProperty<T>()
  +-- addSignal<ARGS...>()

Ipc::ValueOrError<T>  <-- DBus::ValueOrError<T> (alias)
Ipc::Error             <-- DBus::Error (alias)
```

Bridge modules use `mIpcServer->addInterface()` for interface registration and `getDbusServer().emit2<>()` for signal emission. The D-Bus-specific serialization templates remain on the concrete `DBusInterfaceDescription` class. When a TIDL backend is added, it will provide its own `Server`, `Client`, and `InterfaceDescription` implementations.

### PlatformCallbacks (internal/bridge/bridge-platform.h)

Runtime callbacks that decouple the bridge from any specific event loop or toolkit.

```
PlatformCallbacks
  +-- addIdle / removeIdle         # Idle task scheduling
  +-- createTimer / cancelTimer    # Repeating timers
  +-- getToolkitVersion / getAppName
  +-- isAdaptorAvailable
```

## Data Flow

### Bridge Initialization

```
1. Toolkit calls SetPlatformCallbacks(callbacks)
2. Toolkit calls Bridge::GetCurrentBridge() -> creates BridgeImpl singleton
3. Toolkit registers Accessible objects: bridge->AddAccessible(id, accessible)
4. Toolkit calls bridge->Initialize()
   - Reads IsEnabled / ScreenReaderEnabled properties from AT-SPI bus
5. Toolkit calls bridge->ApplicationResumed()
   - Triggers SwitchBridge() -> ForceUp()
   - ForceUp: connects to AT-SPI bus, registers all D-Bus interfaces, embeds socket
6. Bridge is now live, responding to AT-SPI queries from screen readers
```

### AT-SPI Method Call (e.g., GetRole)

```
Screen Reader                    Bridge                         Accessible
    |                              |                               |
    |-- D-Bus: GetRole(path) ----->|                               |
    |                              |-- FindCurrentObject(path) --->|
    |                              |<-- Accessible* --------------|
    |                              |-- accessible->GetRole() ----->|
    |                              |<-- Role::PUSH_BUTTON --------|
    |<-- D-Bus reply: 42 ---------|                               |
```

### Interface Registration (Fallback Pattern)

All bridge modules register their interfaces at path `"/"` with `fallback=true` via the IPC server abstraction. This means the callbacks match any object path. The bridge resolves the actual target Accessible using `FindCurrentObject()`, which extracts the object ID from the IPC request path.

```cpp
// In bridge-accessible.cpp RegisterInterfaces():
mIpcServer->addInterface("/", desc, true);  // fallback=true

// In bridge-base.cpp FindCurrentObject():
// Uses mIpcServer->getCurrentObjectPath() to get the request path
// Path: "/org/a11y/atspi/accessible/1000" -> strips prefix -> "1000"
// Looks up accessible by ID in registry
```

## Mock Architecture

The MockDBusWrapper enables testing the full bridge pipeline in-process:

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

The mock pre-populates responses for external AT-SPI services called during bridge init:

| Service | Method | Response |
|---|---|---|
| org.a11y.Bus | GetAddress | `"unix:path=/tmp/mock-atspi"` |
| org.a11y.atspi.Registry | GetRegisteredEvents | Empty event list |
| org.a11y.atspi.Socket | Embed | Dummy parent Address |
| org.a11y.atspi.Socket | Unembed | Success (no-op) |
| org.a11y.Status | IsEnabled | `true` |
| org.a11y.Status | ScreenReaderEnabled | `true` |

## Build System

The CMake build supports three configurations controlled by `ENABLE_ATSPI` and `eldbus_available`:

```
ENABLE_ATSPI=ON + eldbus:    Full bridge + dbus-tizen.cpp (production)
ENABLE_ATSPI=ON + no eldbus: Full bridge + dbus-stub.cpp  (test/CI)
ENABLE_ATSPI=OFF:            DummyBridge only              (no a11y)
```

### dbus-stub.cpp

Provides the same portable symbols as `dbus-tizen.cpp` (static variables, `DBusClient`/`DBusServer` constructors, `DBusWrapper::Install`/`Installed`) but without the EFL `DefaultDBusWrapper`. Callers must install their own `DBusWrapper` via `Install()` before any D-Bus operations.

### Test Build

`BUILD_TESTS=ON` adds the `accessibility-test` executable. The test compiles all bridge sources directly rather than linking against the shared library, because the library uses `-fvisibility=hidden` which hides internal symbols (`DBusWrapper`, `DBusClient`, etc.) that the test needs to access.

```
cmake .. -DENABLE_ATSPI=ON -DBUILD_TESTS=ON -DENABLE_PKG_CONFIGURE=OFF
make
./accessibility-test
```

### Known Limitations

- `Accessible*` deserialization from D-Bus messages requires `CurrentBridgePtr` context (only available during server-side callback processing). Client-side tests verify the serialized `Address` instead.
- The mock's `addIdle`/`createTimer` callbacks execute synchronously and immediately. This is sufficient for testing but doesn't simulate real async behavior.
