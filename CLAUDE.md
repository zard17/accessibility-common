# Claude Code Instructions for accessibility-common

## Project Overview

Standalone, toolkit-agnostic accessibility library extracted from DALi. Provides the AT-SPI bridge, accessible tree interfaces, and D-Bus serialization layer. Designed to support pluggable IPC backends (D-Bus, TIDL) and be consumable by any UI toolkit.

## Repository Structure

- `accessibility/api/` - Public API: `Accessible`, `Component`, `Bridge` interfaces, enums, types
- `accessibility/public-api/` - Top-level convenience header
- `accessibility/internal/bridge/` - AT-SPI bridge implementation (bridge-base, bridge-impl, bridge-accessible, etc.)
- `accessibility/internal/bridge/dbus/` - D-Bus layer: `dbus.h` (interface + templates), `dbus-tizen.cpp` (EFL backend), `dbus-stub.cpp` (stub backend)
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

- **DBusWrapper virtual interface**: All D-Bus operations go through `DBusWrapper::Installed()`. Swap backends via `DBusWrapper::Install(unique_ptr)`.
- **Fallback interface registration**: Bridge modules register at path `"/"` with `fallback=true`. `FindCurrentObject()` resolves the target from the request path.
- **PlatformCallbacks**: Runtime callbacks decouple the bridge from any event loop or toolkit.
- **Feature system**: `Accessible::AddFeature<T>()` / `GetFeature<T>()` for optional interfaces (Action, EditableText, Value, etc.).

## Important Files

- `dbus/dbus.h` - Core D-Bus abstraction (~2800 lines). Contains `DBusWrapper`, `DBusClient`, `DBusServer`, all serialization templates.
- `bridge-impl.cpp` - Bridge lifecycle: `Initialize()`, `ForceUp()`, `ForceDown()`, `SwitchBridge()`.
- `bridge-base.cpp` - `FindCurrentObject()`, `ApplicationAccessible`, interface registration helpers.
- `accessibility-common.h` - D-Bus signature specializations for `Address`, `Accessible*`, `States`.

## Rules

- Do not commit partial or untested work
- When adding new bridge interfaces, follow the `RegisterInterfaces()` pattern in existing bridge-*.cpp files
- When adding new canned responses to MockDBusWrapper, add them in `SetupCannedResponses()`
- Test executable compiles bridge sources directly (not via shared lib) due to `-fvisibility=hidden`
