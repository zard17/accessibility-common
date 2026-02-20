# Important Files Reference

## IPC Abstraction Layer

- `accessibility/internal/bridge/ipc/ipc-result.h` - Protocol-neutral `Ipc::ValueOrError<T>`, `Ipc::Error`, `Ipc::ErrorType`
- `accessibility/internal/bridge/ipc/ipc-server.h` - Abstract `Ipc::Server` interface for server-side IPC
- `accessibility/internal/bridge/ipc/ipc-client.h` - Abstract `Ipc::Client` interface for client-side IPC
- `accessibility/internal/bridge/ipc/ipc-interface-description.h` - Base class for method/property/signal registration

## D-Bus Backend

- `accessibility/internal/bridge/dbus/dbus-ipc-server.h` - `Ipc::DbusIpcServer` wrapping `DBus::DBusServer`
- `accessibility/internal/bridge/dbus/dbus-ipc-client.h` - `Ipc::DbusIpcClient` wrapping `DBus::DBusClient`
- `accessibility/internal/bridge/dbus/dbus.h` - Core D-Bus abstraction (~2700 lines). Contains `DBusWrapper`, `DBusClient`, `DBusServer`, all serialization templates

## Bridge

- `accessibility/internal/bridge/bridge-impl.cpp` - Bridge lifecycle: `Initialize()`, `ForceUp()`, `ForceDown()`, `SwitchBridge()`
- `accessibility/internal/bridge/bridge-base.cpp` - `FindCurrentObject()`, `ApplicationAccessible`, interface registration helpers
- `accessibility/public-api/accessibility-common.h` - D-Bus signature specializations for `Address`, `Accessible*`, `States`

## Public API

- `accessibility/api/node-proxy.h` - `NodeProxy` abstract interface (42 methods) + `ReadingMaterial`, `NodeInfo`, `RemoteRelation`, `DefaultLabelInfo` structs
- `accessibility/api/app-registry.h` - `AppRegistry` abstract interface for app discovery
- `accessibility/api/accessibility-service.h` - `AccessibilityService` base class (Android-inspired pattern)
- `accessibility/api/accessibility-event.h` - `AccessibilityEvent` struct (10 event types)
- `accessibility/api/gesture-provider.h` - `GestureProvider` abstract interface
- `accessibility/api/tts-engine.h` - `TtsEngine` interface (speak, stop, pause, resume, purge, utterance callbacks) + `SpeakOptions`, `CommandId`
- `accessibility/api/feedback-provider.h` - `FeedbackProvider` interface (playSound, vibrate) + `SoundType` enum (7 types)
- `accessibility/api/reading-composer.h` - `ReadingComposer` class + `ReadingComposerConfig` for TV/mobile
- `accessibility/api/settings-provider.h` - `SettingsProvider` interface + `ScreenReaderSettings` struct (7 fields)
- `accessibility/api/screen-reader-switch.h` - `ScreenReaderSwitch` interface (org.a11y.Status + WM control)
- `accessibility/api/direct-reading-service.h` - `DirectReadingService` interface (org.tizen.DirectReading)
- `accessibility/api/screen-reader-service.h` - `ScreenReaderService` (full mode) + `TvScreenReaderService` (TV mode) headers

## Service Layer

- `accessibility/internal/service/atspi-node-proxy.h` - D-Bus `NodeProxy` implementation (42 methods via `DBus::DBusClient`)
- `accessibility/internal/service/atspi-app-registry.h` - D-Bus `AppRegistry` implementation
- `accessibility/internal/service/composite-app-registry.h` - Merges D-Bus + TIDL registries

## Screen Reader

- `accessibility/internal/service/screen-reader/reading-composer.cpp` - Role/state/description trait composition (~30 roles)
- `accessibility/internal/service/screen-reader/tts-command-queue.h/.cpp` - Pure C++ TTS queue with 300-char chunking, discard policy, pause/resume
- `accessibility/internal/service/screen-reader/symbol-table.h/.cpp` - 53 symbol→spoken text mappings
- `accessibility/internal/service/screen-reader/screen-reader-service.cpp` - Full mode: gesture dispatch, event handling, TTS, feedback
- `accessibility/internal/service/screen-reader/tv-screen-reader-service.cpp` - TV mode: focus-only events, no gestures

## Inspector

- `tools/inspector/inspector-types.h` - Shared `ElementInfo` and `TreeNode` structs used by all inspector engines
- `tools/inspector/query-engine.h` - `InspectorEngine::AccessibilityQueryEngine` class (D-Bus-based tree queries)
- `tools/inspector/direct-query-engine.h` - `InspectorEngine::DirectQueryEngine` class (direct C++ Accessible* queries, no D-Bus)
- `tools/inspector/web-inspector-server.h` - `InspectorEngine::WebInspectorServer` embeddable HTTP server (PIMPL, background thread)
- `tools/inspector/inspector-query-interface.h` - Abstract `InspectorQueryInterface` for NodeProxy-based and Accessible*-based query engines
- `tools/inspector/node-proxy-query-engine.h` - NodeProxy-based tree snapshot engine (thread-safe)
- `accessibility/internal/service/inspector-service.h` - `InspectorService` extends `AccessibilityService` with web inspector

## Screen Reader Demos

- `tools/screen-reader/direct-node-proxy.h` - `NodeProxy` backed by `Accessible*` (in-process, no IPC). 42 methods + CHECKABLE state inference
- `tools/screen-reader/direct-app-registry.h` - `AppRegistry` wrapping DALi root accessible with `weak_ptr` proxy cache
- `tools/screen-reader/mac-tts-engine.h/.mm` - macOS `TtsEngine` using `AVSpeechSynthesizer` (PIMPL + ObjC delegate)
- `tools/screen-reader/screen-reader-demo.cpp` - Gesture-based demo: DALi app + `ScreenReaderService` + keyboard→gesture mapping
- `tools/screen-reader/screen-reader-tv-demo.cpp` - TV focus-based demo: DALi `KeyboardFocusManager` + `TvScreenReaderService` + `FocusChangedSignal`→TTS

## Tests

- `test/mock/mock-node-proxy.h` - Mock `NodeProxy` backed by `TestAccessible*` (no IPC, DFS neighbor navigation)
- `test/mock/mock-app-registry.h` - Mock `AppRegistry` with demo tree and `MockNodeProxy` factory
- `test/test-service.cpp` - 55 unit tests for `AccessibilityService`, `NodeProxy`, navigation, events, gestures
- `test/test-inspector-service.cpp` - 47 unit tests for InspectorService + NodeProxyQueryEngine
- `test/test-screen-reader-service.cpp` - 120 unit tests for ScreenReaderService, TvScreenReaderService, ReadingComposer, TtsCommandQueue, SymbolTable

## Third-Party

- `third-party/cpp-httplib/httplib.h` - Vendored cpp-httplib v0.18.3 (MIT, single-header HTTP server)
