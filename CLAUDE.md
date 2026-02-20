# Claude Code Instructions for accessibility-common

## Project Overview

Standalone, toolkit-agnostic accessibility library extracted from DALi. Provides the AT-SPI bridge, accessible tree interfaces, and D-Bus serialization layer. Designed to support pluggable IPC backends (D-Bus, TIDL) and be consumable by any UI toolkit.

## Repository Structure

- `accessibility/api/` - Public API: `Accessible`, `Component`, `Bridge` interfaces, enums, types, `NodeProxy`, `AppRegistry`, `GestureProvider`, `AccessibilityService`
- `accessibility/public-api/` - Top-level convenience header
- `accessibility/internal/bridge/` - AT-SPI bridge implementation (bridge-base, bridge-impl, bridge-accessible, etc.)
- `accessibility/internal/bridge/ipc/` - IPC abstraction layer: `Ipc::Server`, `Ipc::Client`, `Ipc::InterfaceDescription`, `Ipc::ValueOrError`
- `accessibility/internal/bridge/dbus/` - D-Bus backend: `dbus.h` (serialization templates), `dbus-ipc-server.h` / `dbus-ipc-client.h` (IPC backend wrappers), `dbus-tizen.cpp` (EFL backend), `dbus-stub.cpp` (stub backend)
- `accessibility/internal/service/` - AT-side service layer: `AccessibilityService` base, `AtSpiNodeProxy`, `AtSpiAppRegistry`, `AtSpiEventRouter`, `WindowTracker`, `CompositeAppRegistry`
- `accessibility/internal/service/screen-reader/` - Screen reader services: `ScreenReaderService`, `TvScreenReaderService`, `ReadingComposer`, `TtsCommandQueue`, `SymbolTable`
- `accessibility/internal/service/tidl/` - TIDL scaffold implementations (to be completed on Tizen device)
- `accessibility/internal/service/stub/` - Platform stubs for macOS (no-op gesture, stub registry, stub settings)
- `test/` - Mock D-Bus wrapper, TestAccessible, test application, service tests, mock NodeProxy/AppRegistry/GestureProvider
- `tools/inspector/` - Interactive CLI/Web accessibility inspector (multiple variants: D-Bus, Direct, GDBus)
- `tools/screen-reader/` - Screen reader demos (gesture-based + TV focus-based) with DirectNodeProxy, DirectAppRegistry, MacTtsEngine
- `build/tizen/` - CMake build system

For detailed file descriptions, see [docs/important-files.md](docs/important-files.md).

## Code Style

- 2-space indentation, braces on new lines for namespaces/classes/functions
- `// EXTERNAL INCLUDES` / `// INTERNAL INCLUDES` comment blocks for include grouping
- SCREAMING_SNAKE_CASE for enums, PascalCase for classes, camelCase for methods/variables
- Apache 2.0 license header (Samsung Electronics) on all source files
- Doxygen `@brief` style comments for public API

## Building

```bash
cd build/tizen && mkdir -p build && cd build

# Full build (Tizen with eldbus)
cmake .. -DENABLE_ACCESSIBILITY=ON

# Stub build (macOS/CI without eldbus)
cmake .. -DENABLE_ACCESSIBILITY=ON -DENABLE_PKG_CONFIGURE=OFF

make -j$(nproc)
```

## Tests

All tests build with `-DENABLE_PKG_CONFIGURE=OFF` on macOS:

| Target | CMake Option | Binary | Expected |
|--------|-------------|--------|----------|
| Bridge / core | `-DBUILD_TESTS=ON` | `accessibility-test` | 56 passed |
| AccessibilityService | `-DBUILD_SERVICE_TESTS=ON` | `accessibility-service-test` | 55 passed |
| InspectorService | `-DBUILD_INSPECTOR_SERVICE_TESTS=ON` | `accessibility-inspector-service-test` | 47 passed |
| ScreenReaderService | `-DBUILD_SCREEN_READER_TESTS=ON` | `accessibility-screen-reader-test` | 120 passed |

## Screen Reader Demos (require DALi)

Both demos require `dali2-core`, `dali2-adaptor`, `dali2-toolkit` in `$HOME/tizen/dali-env/lib`.

**Gesture-based** (`-DBUILD_SCREEN_READER_DEMO=ON` → `accessibility-screen-reader-demo`):
Keyboard keys simulate touch gestures. Right/Left = navigate, Enter = activate, Up/Down = adjust ProgressBar.

**TV focus-based** (`-DBUILD_SCREEN_READER_TV_DEMO=ON` → `accessibility-screen-reader-tv-demo`):
DALi `KeyboardFocusManager` handles arrow-key navigation. `FocusChangedSignal` dispatches `STATE_CHANGED(focused)` to `TvScreenReaderService` for TTS. No gesture simulation needed.

```bash
cmake .. -DENABLE_ACCESSIBILITY=ON -DBUILD_SCREEN_READER_TV_DEMO=ON -DENABLE_PKG_CONFIGURE=OFF
make -j$(nproc)
export DYLD_LIBRARY_PATH=$HOME/tizen/dali-env/lib
./accessibility-screen-reader-tv-demo
```

## Accessibility Inspectors

Multiple inspector variants share a common engine. Details in [docs/inspector-architecture.md](docs/inspector-architecture.md).

| Variant | CMake Option | Binary | Description |
|---------|-------------|--------|-------------|
| CLI | `-DBUILD_INSPECTOR=ON` | `accessibility-inspector` | Terminal-based, `p`/`n`/`b`/`s` keys |
| Web | `-DBUILD_WEB_INSPECTOR=ON` | `accessibility-web-inspector` | Browser UI at `:8080` |
| Direct (no D-Bus) | `-DBUILD_WEB_INSPECTOR_DIRECT=ON` | `accessibility-web-inspector-direct` | Direct `Accessible*` queries |
| GDBus (real IPC) | `-DBUILD_WEB_INSPECTOR_GDBUS=ON` | `accessibility-web-inspector-gdbus` | Full D-Bus round-trip |
| Embeddable lib | `-DBUILD_INSPECTOR_LIB=ON` | `libaccessibility-inspector.a` | Link into any DALi app |

## Key Design Patterns

- **IPC abstraction layer**: Bridge modules use `Ipc::Server` / `Ipc::Client` / `Ipc::InterfaceDescription` interfaces. D-Bus is the first backend (`Ipc::DbusIpcServer`, `Ipc::DbusIpcClient`).
- **DBusWrapper virtual interface**: All D-Bus operations go through `DBusWrapper::Installed()`. Swap backends via `DBusWrapper::Install(unique_ptr)`.
- **PlatformCallbacks**: Runtime callbacks decouple the bridge from any event loop or toolkit.
- **Feature system**: `Accessible::AddFeature<T>()` / `GetFeature<T>()` for optional interfaces (Action, EditableText, Value, etc.).
- **AccessibilityService pattern (Android-inspired)**: AT services extend `AccessibilityService` and implement virtual callbacks (`onAccessibilityEvent`, `onWindowChanged`, `onGesture`). The base class provides `navigateNext()`, `navigatePrev()`, `highlightNode()` using `NodeProxy` and `AppRegistry` abstract interfaces.
- **Proxy (not Cache) design**: `NodeProxy` methods are IPC calls (no local tree cache). `getReadingMaterial()` batch call fetches 24 fields in one round-trip.

## Rules

- Do not commit partial or untested work
- When adding new bridge interfaces, follow the `RegisterInterfaces()` pattern in existing bridge-*.cpp files
- When adding new canned responses to MockDBusWrapper, add them in `SetupCannedResponses()`
- Test executable compiles bridge sources directly (not via shared lib) due to `-fvisibility=hidden`

## Documentation
- session 시작 시 Handover.md, docs/architecture-overview.md 읽어서 흐름 파악
- session 종료 시 다음 session에 넘길 내용 Handover.md에 정리, 기존 Handover.md는 지운다
