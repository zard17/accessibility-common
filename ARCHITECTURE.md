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
        dbus/
          dbus.h                  # DBusWrapper virtual interface, DBusClient/Server, serialization
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

All bridge modules register their D-Bus interfaces at path `"/"` with `fallback=true`. This means the callbacks match any object path. The bridge resolves the actual target Accessible using `FindCurrentObject()`, which extracts the object ID from the D-Bus request path.

```cpp
// In bridge-accessible.cpp RegisterInterfaces():
mDbusServer.addInterface("/", desc, true);  // fallback=true

// In bridge-base.cpp FindCurrentObject():
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

`BUILD_TESTS=ON` adds the `accessibility-test` executable that uses MockDBusWrapper to exercise the bridge without real D-Bus.
