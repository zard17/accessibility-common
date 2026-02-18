# Full Stack Data Flow

> Parent document: [architecture-overview.md](architecture-overview.md)

---

## App → Screen Reader (current AT-SPI path)

```mermaid
sequenceDiagram
    participant App as DALi App
    participant CA as ControlAccessible
    participant Bridge as BridgeImpl
    participant IPC as Ipc::Server
    participant TF as TransportFactory
    participant Bus as D-Bus / TIDL
    participant SR as ScreenReaderService
    participant NP as NodeProxy

    App->>CA: SetName("Play")
    CA->>Bridge: Emit(StateChanged)
    Bridge->>IPC: emitSignal("StateChanged", ...)
    IPC->>Bus: send signal
    Bus->>SR: event received
    SR->>SR: onAccessibilityEvent(STATE_CHANGED)
    SR->>NP: getReadingMaterial()
    NP->>Bus: IPC call
    Bus->>Bridge: GetReadingMaterial()
    Bridge->>CA: getName(), getRole(), getStates(), ...
    CA-->>Bridge: {name, role, states, ...}
    Bridge-->>Bus: response
    Bus-->>NP: ReadingMaterial
    NP-->>SR: ReadingMaterial
    SR->>SR: ReadingComposer.compose(material)
    SR->>SR: TtsEngine.speak("Play. Button.")
```

---

## PlatformCallbacks (bridge-platform.h)

Runtime callbacks that decouple the bridge from any specific event loop or toolkit. Set once at initialization via `SetPlatformCallbacks()`.

```
PlatformCallbacks
  +-- addIdle / removeIdle         # Idle task scheduling
  +-- createTimer / cancelTimer    # Repeating timers
  +-- getToolkitVersion / getAppName
  +-- isAdaptorAvailable
```

---

## Bridge Initialization

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

---

## AT-SPI Method Call (e.g., GetRole)

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

---

## Gesture → Navigation

```mermaid
sequenceDiagram
    participant WM as Window Manager
    participant GP as GestureProvider
    participant SR as ScreenReaderService
    participant SVC as AccessibilityService (base)
    participant NP as NodeProxy
    participant Bus as IPC
    participant Bridge as App Bridge

    WM->>GP: ONE_FINGER_FLICK_RIGHT
    GP->>SR: onGesture(FLICK_RIGHT)
    SR->>SVC: navigateNext()
    SVC->>NP: getNeighbor(root, current, FORWARD)
    NP->>Bus: IPC call
    Bus->>Bridge: GetNeighbor(root, current, FORWARD)
    Bridge-->>Bus: next node address
    Bus-->>NP: next NodeProxy
    NP-->>SVC: next
    SVC->>NP: grabHighlight()
    NP->>Bus: GrabHighlight()
    Bus->>Bridge: GrabHighlight()
    Bridge->>Bridge: render highlight
    SVC-->>SR: highlighted node
    SR->>SR: ReadingComposer + TtsEngine.speak()
```
