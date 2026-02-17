# Claude Code Instructions for accessibility-common

## Project Overview

Standalone, toolkit-agnostic accessibility library extracted from DALi. Provides the AT-SPI bridge, accessible tree interfaces, and D-Bus serialization layer. Designed to support pluggable IPC backends (D-Bus, TIDL) and be consumable by any UI toolkit.

## Repository Structure

- `accessibility/api/` - Public API: `Accessible`, `Component`, `Bridge` interfaces, enums, types
- `accessibility/public-api/` - Top-level convenience header
- `accessibility/internal/bridge/` - AT-SPI bridge implementation (bridge-base, bridge-impl, bridge-accessible, etc.)
- `accessibility/internal/bridge/ipc/` - IPC abstraction layer: `Ipc::Server`, `Ipc::Client`, `Ipc::InterfaceDescription`, `Ipc::ValueOrError`
- `accessibility/internal/bridge/dbus/` - D-Bus backend: `dbus.h` (serialization templates), `dbus-ipc-server.h` / `dbus-ipc-client.h` (IPC backend wrappers), `dbus-tizen.cpp` (EFL backend), `dbus-stub.cpp` (stub backend)
- `test/` - Mock D-Bus wrapper, TestAccessible, test application
- `build/tizen/` - CMake build system

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
cmake .. -DENABLE_ATSPI=ON

# Stub build (macOS/CI without eldbus)
cmake .. -DENABLE_ATSPI=ON -DENABLE_PKG_CONFIGURE=OFF

# With tests
cmake .. -DENABLE_ATSPI=ON -DBUILD_TESTS=ON -DENABLE_PKG_CONFIGURE=OFF

make -j$(nproc)
```

## Running Tests

```bash
./accessibility-test
# Expected: "=== Results: 31 passed, 0 failed ==="
```

## Key Design Patterns

- **IPC abstraction layer**: Bridge modules use `Ipc::Server` / `Ipc::Client` / `Ipc::InterfaceDescription` interfaces. D-Bus is the first backend (`Ipc::DbusIpcServer`, `Ipc::DbusIpcClient`). `Ipc::ValueOrError<T>` is protocol-neutral; `DBus::ValueOrError<T>` is a backward-compat alias.
- **DBusWrapper virtual interface**: All D-Bus operations go through `DBusWrapper::Installed()`. Swap backends via `DBusWrapper::Install(unique_ptr)`.
- **Fallback interface registration**: Bridge modules register at path `"/"` with `fallback=true` via `mIpcServer->addInterface()`. `FindCurrentObject()` resolves the target from the request path.
- **PlatformCallbacks**: Runtime callbacks decouple the bridge from any event loop or toolkit.
- **Feature system**: `Accessible::AddFeature<T>()` / `GetFeature<T>()` for optional interfaces (Action, EditableText, Value, etc.).

## Important Files

- `ipc/ipc-result.h` - Protocol-neutral `Ipc::ValueOrError<T>`, `Ipc::Error`, `Ipc::ErrorType`.
- `ipc/ipc-server.h` - Abstract `Ipc::Server` interface for server-side IPC.
- `ipc/ipc-client.h` - Abstract `Ipc::Client` interface for client-side IPC.
- `ipc/ipc-interface-description.h` - Base class for method/property/signal registration.
- `dbus/dbus-ipc-server.h` - `Ipc::DbusIpcServer` wrapping `DBus::DBusServer`.
- `dbus/dbus-ipc-client.h` - `Ipc::DbusIpcClient` wrapping `DBus::DBusClient`.
- `dbus/dbus.h` - Core D-Bus abstraction (~2700 lines). Contains `DBusWrapper`, `DBusClient`, `DBusServer`, all serialization templates.
- `bridge-impl.cpp` - Bridge lifecycle: `Initialize()`, `ForceUp()`, `ForceDown()`, `SwitchBridge()`.
- `bridge-base.cpp` - `FindCurrentObject()`, `ApplicationAccessible`, interface registration helpers.
- `accessibility-common.h` - D-Bus signature specializations for `Address`, `Accessible*`, `States`.

## Rules

- Do not commit partial or untested work
- When adding new bridge interfaces, follow the `RegisterInterfaces()` pattern in existing bridge-*.cpp files
- When adding new canned responses to MockDBusWrapper, add them in `SetupCannedResponses()`
- Test executable compiles bridge sources directly (not via shared lib) due to `-fvisibility=hidden`

## Documentation
- session 시작 시 PLAN.md 읽어서 흐름 파악
- session 종료 시 다음 session에 넘길 내용 Handover.md에 정리, 기존 Handover.md는 지운다