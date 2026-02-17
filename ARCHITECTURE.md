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
  tools/
    inspector/
      query-engine.h/.cpp         # Shared AccessibilityQueryEngine (bridge setup, queries, navigation)
      inspector.cpp               # CLI inspector (thin wrapper over query-engine)
      web-inspector.cpp           # Web inspector HTTP server with REST API
      web-inspector-resources.h   # Embedded HTML/CSS/JS frontend (raw string literal)
      tts.h                       # TTS interface header
      tts-mac.mm                  # macOS TTS (AVSpeechSynthesizer)
      tts-stub.cpp                # Fallback TTS (print to console)
  third-party/
    cpp-httplib/
      httplib.h                   # Vendored cpp-httplib v0.18.3 (MIT, single-header HTTP server)
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
  +-- emitSignal()                  Ipc::SignalVariant
       |                              = variant<int, string, Address, Rect<int>>
       v                                   |
Ipc::DbusIpcServer                  Ipc::DbusIpcClient
  wraps DBus::DBusServer              wraps DBus::DBusClient
  +-- emitSignal() -> emit2<>()       +-- getDbusClient()
  +-- getDbusServer() [internal]
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

Bridge modules use `mIpcServer->addInterface()` for interface registration and `mIpcServer->emitSignal()` for signal emission. Signal payloads use `Ipc::SignalVariant` (a `std::variant` of int, string, Address, Rect<int>), which `DbusIpcServer` maps to `emit2<EldbusVariant<T>>()` calls via `std::visit`. The `getDbusServer()` method is private to `BridgeBase` and not accessible from bridge modules.

The D-Bus-specific serialization templates remain on the concrete `DBusInterfaceDescription` class. When a TIDL or NSAccessibility backend is added, it will provide its own `Server`, `Client`, and `InterfaceDescription` implementations.

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

### Inspector Build

Both inspectors compile bridge sources directly (like tests) and use MockDBusWrapper for in-process IPC.

```
# CLI inspector
cmake .. -DENABLE_ATSPI=ON -DBUILD_INSPECTOR=ON -DENABLE_PKG_CONFIGURE=OFF
make && ./accessibility-inspector

# Web inspector
cmake .. -DENABLE_ATSPI=ON -DBUILD_WEB_INSPECTOR=ON -DENABLE_PKG_CONFIGURE=OFF
make && ./accessibility-web-inspector
```

## Inspector Architecture

Both the CLI and web inspectors share a common engine (`AccessibilityQueryEngine`) that encapsulates bridge initialization, demo tree construction, D-Bus queries, and navigation.

```
                    AccessibilityQueryEngine
                   (query-engine.h/.cpp)
                    |                    |
          +---------+----------+   +----+-----+
          |                    |   |          |
   CLI Inspector        Web Inspector    (future consumers)
   (inspector.cpp)      (web-inspector.cpp)
          |                    |
   stdin/stdout         HTTP REST API
                        (cpp-httplib)
                              |
                         Browser UI
                   (web-inspector-resources.h)
```

### AccessibilityQueryEngine (tools/inspector/query-engine.h/.cpp)

Reusable engine that initializes a demo accessible tree and provides query/navigation methods. Consumers call engine methods instead of making raw D-Bus queries.

```
AccessibilityQueryEngine
  +-- Initialize()              # MockDBusWrapper + PlatformCallbacks + bridge + demo tree
  +-- Shutdown()                # Bridge teardown
  +-- GetRootId() / GetFocusedId() / SetFocusedId()
  +-- GetElementInfo(id)        # Returns ElementInfo (name, role, states, bounds, children)
  +-- BuildTree(rootId)         # Returns TreeNode hierarchy
  +-- Navigate(id, forward)     # Forward/backward via bridge GetNeighbor
  +-- NavigateChild(id)         # First child
  +-- NavigateParent(id)        # Parent
```

Internally, the engine uses `DBusClient` to query the bridge's registered AT-SPI interfaces through MockDBusWrapper — the same code path as a real screen reader.

```
AccessibilityQueryEngine
  |
  +-- DBusClient.method("GetRole").call()
  |     |
  v     v
Bridge (BridgeImpl)
  |
  +-- Registered AT-SPI interfaces (same code as production)
  |
  v
MockDBusWrapper (in-process routing)
```

### Demo Tree

The engine builds a demo tree modeling a Tizen media player app:

```
[WINDOW] "Main Window"                       <- ACTIVE
  [PANEL] "Header"
    [PUSH_BUTTON] "Menu"                     <- FOCUSABLE + HIGHLIGHTABLE
    [LABEL] "My Tizen App"                   <- HIGHLIGHTABLE (not FOCUSABLE)
  [PANEL] "Content"
    [PUSH_BUTTON] "Play"                     <- FOCUSABLE + HIGHLIGHTABLE
    [SLIDER] "Volume"                        <- FOCUSABLE + HIGHLIGHTABLE
    [LABEL] "Now Playing: Bohemian Rhapsody" <- HIGHLIGHTABLE (not FOCUSABLE)
  [PANEL] "Footer"
    [PUSH_BUTTON] "Previous"                 <- FOCUSABLE + HIGHLIGHTABLE
    [PUSH_BUTTON] "Next"                     <- FOCUSABLE + HIGHLIGHTABLE
```

**FOCUSABLE vs HIGHLIGHTABLE**: Buttons and sliders are both FOCUSABLE (can receive keyboard focus) and HIGHLIGHTABLE (navigable by screen reader cursor). Labels are HIGHLIGHTABLE only — the screen reader can navigate to them and announce their text, but they don't accept keyboard focus. The bridge's `GetNeighbor()` walks elements with HIGHLIGHTABLE state; `IsObjectAcceptable()` in `bridge-accessible.cpp` checks VISIBLE + HIGHLIGHTABLE.

### CLI Inspector (tools/inspector/inspector.cpp)

Thin interactive wrapper (~220 lines) over `AccessibilityQueryEngine`. Provides single-key commands for tree exploration and TTS:

| Key | Action |
|-----|--------|
| `p` | Print accessibility tree (`>>` marks focused element) |
| `n`/`b` | Navigate forward/backward |
| `c`/`u` | Navigate to first child / parent |
| `r` | Read current element details |
| `s` | Speak current element via system TTS |
| `h` | Show help |
| `q` | Quit |

TTS uses `AVSpeechSynthesizer` on macOS (`tts-mac.mm`) and prints to console elsewhere (`tts-stub.cpp`).

### Web Inspector (tools/inspector/web-inspector.cpp)

Browser-based GUI served via embedded HTTP server (~230 lines). Uses cpp-httplib (single-header, MIT licensed, vendored at `third-party/cpp-httplib/httplib.h`).

**REST API:**

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Serves embedded HTML/CSS/JS page |
| `/api/tree` | GET | Full tree JSON + current focusedId |
| `/api/element/:id` | GET | Element details (name, role, states, bounds, children, parent) |
| `/api/navigate` | POST | Navigate: `{"direction": "next\|prev\|child\|parent"}` |

**Frontend** (`web-inspector-resources.h`): Embedded as a C++ raw string literal (`R"HTMLPAGE(...)HTMLPAGE"`) for single-binary deployment. Features:
- Dark Catppuccin-themed two-panel layout (tree + detail)
- Click-to-select tree nodes with collapse/expand
- Navigation buttons: Prev, Next, Child, Parent, Refresh
- Keyboard shortcuts: Tab/Shift+Tab, Enter, Backspace, S (speak), R (refresh)
- TTS via Web Speech API (browser-side, no server dependency)

A mutex protects the engine from concurrent HTTP request handlers. JSON is serialized manually (no external JSON library — only 3 endpoints).

### Known Limitations

- `Accessible*` deserialization from D-Bus messages requires `CurrentBridgePtr` context (only available during server-side callback processing). Client-side tests verify the serialized `Address` instead.
- The mock's `addIdle`/`createTimer` callbacks execute synchronously and immediately. This is sufficient for testing but doesn't simulate real async behavior.
